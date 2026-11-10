
#include "dialog_speech_internal.h"

#include "engine_game.h"      // IsKotor2 — per-game appearance mask + tag lists
#include "hotkeys.h"          // Action::DialogRepeatLine (R re-speaks the line)
#include "engine_manager.h"   // kAddrGuiManagerPtr, kMgrPanels*
#include "engine_offsets.h"
#include "engine_panels.h"    // PanelKind, IdentifyPanel
#include "engine_reads.h"
#include "log.h"
#include "menus_extract.h"    // FromControl
#include "menus_modsettings.h" // GetToggle(Option::HumanSubtitles)
#include "speech.h"
#include "strings.h"          // DialogRepeatLineHint — R-repeat cue for forced lines
#include "transitions.h"      // IsModuleLoadPending — gate during cutscene-load
#include "tutorial_hints.h"   // HintForDialogLine — detect a rewritten tutorial line
#include "locked_recall.h"    // MaybeCapture — story-locked-object bark recall
#include "tutorial_popup.h"   // RecordPendingHint — fire a popup at the reply break
                              // transient (engine LYT loader use-after-free)

namespace acc::dialog_speech {

namespace {

// Find the foreground active dialog panel — first match for any of the
// dialog kinds. Returns the panel pointer + which kind matched.
struct DialogPanelMatch {
    void*                 panel;
    acc::engine::PanelKind kind;
};

DialogPanelMatch FindActiveDialogPanel() {
    DialogPanelMatch out{nullptr, acc::engine::PanelKind::Unknown};
    // SEH-guarded: this walks the manager's panels[], which the engine is
    // tearing down mid-handoff during a cutscene transition (see the
    // module-load latch note in Tick() below). The null checks here cannot
    // catch a stale or freed panel pointer, only an absent one. Callers
    // that go through Tick() are additionally gated by
    // transitions::IsModuleLoadPending, but this function is not
    // Tick()-only, so it guards itself.
    constexpr int kCap = 32;
    void* panelData[kCap];
    int n = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panelData, kCap);
    __try {
        for (int i = 0; i < n; ++i) {
            void* p = panelData[i];
            if (!p) continue;
            auto k = acc::engine::IdentifyPanel(p);
            switch (k) {
            case acc::engine::PanelKind::DialogCinematic:
            case acc::engine::PanelKind::DialogCinematicCopy:
            case acc::engine::PanelKind::DialogComputer:
            case acc::engine::PanelKind::DialogComputerCamera:
                out.panel = p;
                out.kind  = k;
                return out;
            default:
                break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out.panel = nullptr;
        out.kind  = acc::engine::PanelKind::Unknown;
    }
    return out;
}

// Find a BarkBubble panel (independent lifecycle from main dialog).
void* FindBarkBubblePanel() {
    return acc::engine::FindPanelByKind(acc::engine::PanelKind::BarkBubble);
}

// Read row count of a CSWGuiListBox at panel + offset.
int ReadListBoxRowCount(void* panel, size_t lbOffset) {
    if (!panel) return 0;
    __try {
        void* lb = reinterpret_cast<unsigned char*>(panel) + lbOffset;
        auto* lbList = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(lb) +
            kListBoxControlsOffset);
        if (!lbList || !lbList->data) return 0;
        return lbList->size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// Walk a panel's controls[] for the first child whose text resolves
// non-empty. Used as a fallback for the BarkBubble — its label offset
// isn't documented in the plan, but the panel renders a single visible
// text label which reads via FromControl.
bool ReadFirstVisibleText(void* panel, char* outBuf, size_t bufSize) {
    if (!panel || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    __try {
        auto* base = reinterpret_cast<unsigned char*>(panel);
        auto* controls = reinterpret_cast<CExoArrayList*>(
            base + kPanelControlsOffset);
        if (!controls || !controls->data) return false;
        int sz = controls->size > 16 ? 16 : controls->size;
        for (int i = 0; i < sz; ++i) {
            void* child = controls->data[i];
            if (!child) continue;
            if (acc::menus::extract::FromControl(child, outBuf, bufSize) &&
                outBuf[0] != '\0') {
                return true;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
    }
    return false;
}

// For the Computer dialog variant, also speak the message_listbox
// (terminal output) when its row count grows. DialogComputer ONLY:
// kDialogComputerMessageListBoxOffset is witnessed solely in the
// CSWGuiDialogComputer constructor (both games), and the
// CSWGuiDialogComputerCamera object does not extend that far. Reading
// the same offset off a camera panel walked garbage "row" pointers
// into engine code bytes — a multi-second SEH storm that spoke a TLK
// compiler-error string and ended in a dead process
// (userlogs/kennycrash0782408: K2 Peragus dormitory console,
// patch-20260823-195331.log 00:35:38, WER dump EIP=0xFFFFFFFE at
// 00:35:44). The IsListBox vtable check is the same gate the reply
// monitor applies before trusting its fixed offset, so a future
// layout drift degrades to silence instead of a garbage walk.
void TickComputerRows(void* panel) {
    // Baseline is per panel: carrying a stale high-water mark into
    // the next terminal would swallow its first rows.
    static void* s_lastComputerPanel = nullptr;
    static int   s_lastComputerRows = 0;
    if (s_lastComputerPanel != panel) {
        s_lastComputerPanel = panel;
        s_lastComputerRows  = 0;
    }
    int rows = ReadListBoxRowCount(panel,
                                   kDialogComputerMessageListBoxOffset);
    if (rows > s_lastComputerRows) {
        // Speak each newly appended terminal line.
        __try {
            void* lb = reinterpret_cast<unsigned char*>(panel) +
                       kDialogComputerMessageListBoxOffset;
            auto* lbList = reinterpret_cast<CExoArrayList*>(
                reinterpret_cast<unsigned char*>(lb) +
                kListBoxControlsOffset);
            if (lbList && lbList->data) {
                int cap = rows > lbList->size ? lbList->size : rows;
                for (int i = s_lastComputerRows;
                     i < cap; ++i) {
                    void* row = lbList->data[i];
                    if (!row) continue;
                    char text[512];
                    if (acc::menus::extract::FromControl(
                            row, text, sizeof(text)) &&
                        text[0] != '\0') {
                        acc::speech::Speak(text, /*interrupt=*/false);
                        acclog::Write("Dialog.Speech",
                                      "computer row %d [%.200s]",
                                      i, text);
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // ignore — try again next tick
        }
    }
    if (rows < s_lastComputerRows) {
        // Reset on shrink (panel cleared).
        acclog::Write("Dialog.Speech",
                      "computer rows reset %d -> %d",
                      s_lastComputerRows, rows);
    }
    s_lastComputerRows = rows;
}

// ---- NPC line + replies count for the main dialog panel. ----
void TickDialogPanel() {
    DialogPanelMatch m = FindActiveDialogPanel();
    static void*       s_lastPanel    = nullptr;
    static char        s_lastNpcLine[512] = {0};

    if (!m.panel) {
        if (s_lastPanel) {
            acclog::Write("Dialog.Speech", "panel closed");
            s_lastPanel = nullptr;
            s_lastNpcLine[0] = '\0';
            // A Trask Q&A branch (e.g. "How do I open doors?") can record
            // its keyboard hint and then return straight to the world with no
            // further reply break — so the reply-break trigger in
            // MonitorDialogReplies never flushed it. Flush on dialogue close so
            // the recap popup still fires. Idempotent: no-ops if nothing pending
            // or a popup is already up (it fired at an earlier reply break).
            acc::tutorial_popup::FirePendingAtReplyBreak();
        }
        return;
    }

    // First sight — set baseline silently. Don't replay history on
    // open; the very first NPC line will trigger via the diff path
    // on the next tick when the engine populates the message label.
    if (s_lastPanel != m.panel) {
        s_lastPanel = m.panel;
        s_lastNpcLine[0] = '\0';
    }

    char npc[512] = "";
    bool gotNpc = acc::engine::ReadLabelTextAt(m.panel, kDialogMessageLabelOffset,
                                npc, sizeof(npc));
    if (gotNpc && std::strcmp(npc, s_lastNpcLine) != 0) {
        // Rewritten tutorial line (Trask / pop): keep it suppressed during
        // his Basic VO, but remember its keyboard hint + source strref so
        // the reply-prompt monitor can pop a real tutorial window with it
        // once he's finished speaking. Recorded regardless of suppression.
        uint32_t tutStrref = 0;
        bool tutorialLine = false;
        if (const char* tutHint =
                acc::tutorial_hints::HintForDialogLine(npc, &tutStrref)) {
            acc::tutorial_popup::RecordPendingHint(tutStrref, tutHint);
            tutorialLine = true;
        }

        // Per-line speaker classification: log race + appearance even
        // when we end up speaking, so future override decisions can
        // be made on real (speaker tag, race, appearance_type) data.
        SpeakerInfo info;
        bool resolved     = ResolveDialogSpeaker(info);
        bool suppressible = resolved && IsSuppressibleSpeaker(info);
        // KOTOR 2's tutorial lines are spoken by T3-M4 — a droid, whose
        // subtitles are normally force-spoken — so the human-VO
        // suppression that keeps Trask's rewritten lines quiet on KOTOR 1
        // never fires for them. Suppress a matched tutorial line
        // explicitly there: the popup at the reply break carries the
        // rewritten content. (On KOTOR 1 the Trask set is human-voiced
        // and already suppressible; adding the term there would only
        // change behaviour with the HumanSubtitles toggle on, so K1 keeps
        // its shipped semantics.)
        bool tutorialManaged = tutorialLine && acc::game::IsKotor2();
        // A curated VO-less subtitle line (e.g. Trask's Endar Spire level-up
        // gate, strref 39454) ships with no voice-over, so the human-subtitle
        // suppression would silence it entirely. Force those specific lines
        // back into the spoken channel regardless of the toggle.
        bool forcedSpoken = acc::tutorial_hints::IsForcedSpokenDialogLine(npc);
        // The inverse curation: a line with real recorded VO whose speaker
        // the classifier cannot resolve (the K2 prologue narrator renders
        // as cinematic-copy panels with no speaker object), so the
        // human-VO suppression never fires and the subtitle would talk
        // over the actor.
        bool voicedLine = acc::tutorial_hints::IsVoicedSuppressedLine(npc);
        bool suppress = (suppressible || tutorialManaged || voicedLine) &&
            !forcedSpoken &&
            !acc::menus::modsettings::GetToggle(
                acc::menus::modsettings::Option::HumanSubtitles);

        // When suppressing, still surface a leading skill-check outcome
        // marker ([Success]/[Failure] etc.) so a Persuade/Computer/Repair
        // result reaches the player without the redundant Basic VO line.
        char marker[80] = "";
        bool spokeMarker = false;
        if (!suppress) {
            // A forced VO-less line queues (interrupt=false) so it lands
            // AFTER a cue already in flight from the same interaction — the
            // "This object is locked" feedback that triggered this
            // dialogue — instead of clobbering it. Normal lines interrupt.
            acc::speech::Speak(npc, /*interrupt=*/!forcedSpoken);
            // The line is silent unless force-spoken, so tell the player once
            // that R re-speaks it — their only other way to hear it. Queued
            // after the line itself.
            if (forcedSpoken) {
                acc::speech::Speak(
                    acc::strings::Get(acc::strings::Id::DialogRepeatLineHint),
                    /*interrupt=*/false);
            }
        } else if (ExtractSkillCheckMarker(npc, marker, sizeof(marker))) {
            acc::speech::Speak(marker, /*interrupt=*/true);
            spokeMarker = true;
        }
        acclog::Write("Dialog.Speech",
                      "NPC line panel=%p kind=%s speaker=[%s] "
                      "appearance=%d race=%d suppressible=%d forced=%d "
                      "tutorial=%d voiced=%d suppress=%d marker=[%s] "
                      "-> [%.300s]",
                      m.panel, acc::engine::PanelKindName(m.kind),
                      resolved ? info.tag : "?",
                      info.appearance, info.raceEnum,
                      suppressible ? 1 : 0, forcedSpoken ? 1 : 0,
                      tutorialManaged ? 1 : 0, voicedLine ? 1 : 0,
                      suppress ? 1 : 0,
                      spokeMarker ? marker : "", npc);
        std::strncpy(s_lastNpcLine, npc, sizeof(s_lastNpcLine) - 1);
        s_lastNpcLine[sizeof(s_lastNpcLine) - 1] = '\0';
    }

    if (m.kind == acc::engine::PanelKind::DialogComputer &&
        acc::engine::IsListBox(
            reinterpret_cast<unsigned char*>(m.panel) +
            kDialogComputerMessageListBoxOffset)) {
        TickComputerRows(m.panel);
    }

    // ---- R — re-speak the current NPC line on demand. ----
    // Gated on a dialog panel being foreground (we're inside the active
    // branch). Repeats the last captured line verbatim, even if it was
    // VO-suppressed — pressing R is an explicit "say it again" request,
    // so the suppression toggle doesn't apply here.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::DialogRepeatLine) &&
        s_lastNpcLine[0] != '\0') {
        acc::speech::Speak(s_lastNpcLine, /*interrupt=*/true);
        acclog::Write("Dialog.Speech", "R repeat -> [%.300s]", s_lastNpcLine);
    }
}

// ---- Security-camera feed — its own panel, its own lifecycle. ----
//
// Using a console's "view camera" option swaps the terminal for
// CSWGuiDialogComputerCamera: a live camera picture with exactly one
// control on it, LBL_RETURN, whose text is the engine's own localised
// "press Enter to cancel the live feed and return" (computercamera.gui
// strref 48226 on KOTOR 1, computercam_p.gui strref 48759 on KOTOR 2 —
// both games ship a one-control panel). Nothing else on it is text, so
// that label IS the accessible content: without it the screen goes to a
// picture the player cannot see, in silence, with no hint of the key that
// leaves it.
//
// Deliberately NOT read through FindActiveDialogPanel: that returns the
// first dialog-kind panel in panels[], and while a console is open the
// camera panel and the conversation panel trade places there from tick to
// tick, so keying off it would re-announce on every swap. An edge on the
// camera panel's own presence fires exactly once per feed.
//
// Read the label rather than shipping our own string: the engine's text is
// already in the player's language, and it names the key the ENGINE
// actually honours, so it cannot drift out of sync with a rebind.
void TickCameraFeed() {
    void* cam = acc::engine::FindPanelByKind(
        acc::engine::PanelKind::DialogComputerCamera);
    static void* s_lastCamPanel = nullptr;
    if (!cam) {
        s_lastCamPanel = nullptr;
    } else if (cam != s_lastCamPanel) {
        s_lastCamPanel = cam;
        char hint[256] = "";
        if (ReadFirstVisibleText(cam, hint, sizeof(hint)) && hint[0]) {
            acc::speech::Speak(hint, /*interrupt=*/true);
            acclog::Write("Dialog.Speech",
                          "camera feed opened panel=%p -> [%.200s]",
                          cam, hint);
        } else {
            // Panel is up but carries no readable label — say so rather
            // than leaving the player in a silent picture with no idea a
            // screen change happened at all.
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::CameraFeed),
                          /*interrupt=*/true);
            acclog::Write("Dialog.Speech",
                          "camera feed opened panel=%p — no readable label, "
                          "spoke the generic cue", cam);
        }
    }
}

// ---- BarkBubble — independent lifecycle. ----
void TickBark() {
    void* bark = FindBarkBubblePanel();
    static void* s_lastBark = nullptr;
    static char  s_lastBarkText[512] = {0};
    if (!bark) {
        if (s_lastBark) {
            acclog::Write("Dialog.Speech", "bark closed");
            s_lastBark = nullptr;
            s_lastBarkText[0] = '\0';
            // KOTOR 2's prologue delivers several tutorial lines as narrator
            // barks (fl_bash, map, blast, ...). Their hints are recorded
            // below; a bark has no reply break, so its close is the "VO has
            // finished" moment — flush the pending popup here, mirroring the
            // dialogue-close flush above. Idempotent: no-ops when nothing is
            // pending or a popup is already up.
            acc::tutorial_popup::FirePendingAtReplyBreak();
        }
        return;
    }

    char text[512] = "";
    ReadFirstVisibleText(bark, text, sizeof(text));
    if (text[0] != '\0' && std::strcmp(text, s_lastBarkText) != 0) {
        // Tutorial line delivered as a bark (KOTOR 2 prologue): record its
        // hint for the popup fired at bark close, and keep the raw
        // mouse/pad wording out of the spoken channel — the popup carries
        // the rewritten content.
        uint32_t tutStrref = 0;
        const char* tutHint =
            acc::tutorial_hints::HintForDialogLine(text, &tutStrref);
        if (tutHint) {
            acc::tutorial_popup::RecordPendingHint(tutStrref, tutHint);
        }
        bool tutorialManaged = (tutHint != nullptr) && acc::game::IsKotor2();
        // Barks carry their own speaker: CSWGuiBarkBubble.object_id names
        // the owning creature (client id). Ownerless barks (system /
        // loudspeaker feedback) resolve false and stay spoken; a human or
        // droid owner classifies the same way dialog does, so barks now
        // honour the HumanSubtitles toggle instead of always speaking.
        SpeakerInfo info;
        bool resolved     = ResolveBarkSpeaker(bark, info);
        bool suppressible = resolved && IsSuppressibleSpeaker(info);
        // Curated voiced lines whose speaker cannot be resolved (the K2
        // prologue narrator barks ownerless) — suppress like any voiced
        // speaker so the subtitle doesn't talk over the actor.
        bool voicedLine = acc::tutorial_hints::IsVoicedSuppressedLine(text);
        bool suppress = (suppressible || tutorialManaged || voicedLine) &&
            !acc::menus::modsettings::GetToggle(
                acc::menus::modsettings::Option::HumanSubtitles);

        if (!suppress) {
            acc::speech::Speak(text, /*interrupt=*/false);
        }
        // Story-locked doors/containers explain themselves via an ownerless
        // bark that fires once. Offer it for capture so a later locked
        // attempt on the same object can replay the reason (see
        // locked_recall.h). Ownerless == resolved false.
        acc::locked_recall::MaybeCapture(text, /*ownerless=*/!resolved);
        acclog::Write("Dialog.Speech",
                      "bark panel=%p speaker=[%s] appearance=%d race=%d "
                      "suppressible=%d tutorial=%d voiced=%d suppress=%d "
                      "-> [%.300s]",
                      bark, resolved ? info.tag : "?",
                      info.appearance, info.raceEnum,
                      suppressible ? 1 : 0, tutorialManaged ? 1 : 0,
                      voicedLine ? 1 : 0, suppress ? 1 : 0, text);
        std::strncpy(s_lastBarkText, text, sizeof(s_lastBarkText) - 1);
        s_lastBarkText[sizeof(s_lastBarkText) - 1] = '\0';
    }
    s_lastBark = bark;
}

}  // namespace

void Tick() {
    // Module-load latch — FindActiveDialogPanel walks the manager's
    // panels[] which the engine is tearing down mid-handoff during a
    // cutscene transition. See transitions.h IsModuleLoadPending.
    if (acc::transitions::IsModuleLoadPending()) return;

    TickDialogPanel();
    TickCameraFeed();
    TickBark();
}

}  // namespace acc::dialog_speech
