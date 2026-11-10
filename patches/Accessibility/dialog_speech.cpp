// Dialog narration, polled each tick (see dialog_speech.h).

#include "dialog_speech_internal.h"

#include "dialog_speech_marker.h"
#include "engine_game.h"
#include "hotkeys.h"
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_extract.h"
#include "menus_modsettings.h"
#include "speech.h"
#include "strings.h"
#include "transitions.h"
#include "tutorial_hints.h"
#include "locked_recall.h"
#include "tutorial_popup.h"

namespace acc::dialog_speech {

namespace {

struct DialogPanelMatch {
    void*                 panel;
    acc::engine::PanelKind kind;
};

// The first dialog-kind panel in the manager's panels[]. SEH-guarded: the
// engine tears panels[] down mid-handoff during a cutscene transition, and
// a null check cannot catch a freed panel pointer.
DialogPanelMatch FindActiveDialogPanel() {
    DialogPanelMatch out{nullptr, acc::engine::PanelKind::Unknown};
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

// The text of a panel's first child that has any. The bark bubble and the
// camera feed each render a single text label.
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

// Records a tutorial line's keyboard hint for the popup fired at the next
// reply break or bark close. True when the line itself should stay silent:
// on KOTOR 2 the tutorial lines come from T3-M4 (a droid) and narrator barks,
// which would otherwise be spoken, and the popup carries the rewritten
// content. KOTOR 1's tutorial lines are Trask's, human-voiced and already
// suppressible.
bool RecordTutorialLine(const char* line) {
    uint32_t strref = 0;
    const char* hint = acc::tutorial_hints::HintForDialogLine(line, &strref);
    if (!hint) return false;
    acc::tutorial_popup::RecordPendingHint(strref, hint);
    return acc::game::IsKotor2();
}

// A line stays silent when its speaker's voice-over already says it, when
// the tutorial popup carries it, or when it is a curated voiced line whose
// speaker can't be resolved (the KOTOR 2 prologue narrator); unless it is
// force-spoken or the HumanSubtitles toggle is on.
bool SilenceLine(bool suppressible, bool tutorialManaged, bool voicedLine,
                 bool forcedSpoken) {
    return (suppressible || tutorialManaged || voicedLine) && !forcedSpoken &&
           !acc::menus::modsettings::GetToggle(
               acc::menus::modsettings::Option::HumanSubtitles);
}

// A computer terminal's output rows, spoken as they are appended. Only on
// DialogComputer: kDialogComputerMessageListBoxOffset is witnessed in the
// CSWGuiDialogComputer constructor alone, and the camera panel does not
// extend that far; reading it there walked garbage row pointers into engine
// code and killed the process. The caller's IsListBox check makes a future
// layout drift fall silent instead.
void TickComputerRows(void* panel) {
    // Per panel: a stale high-water mark would swallow the next terminal's
    // first rows.
    static void* s_lastComputerPanel = nullptr;
    static int   s_lastComputerRows = 0;
    if (s_lastComputerPanel != panel) {
        s_lastComputerPanel = panel;
        s_lastComputerRows  = 0;
    }
    int rows = ReadListBoxRowCount(panel,
                                   kDialogComputerMessageListBoxOffset);
    if (rows > s_lastComputerRows) {
        __try {
            void* lb = reinterpret_cast<unsigned char*>(panel) +
                       kDialogComputerMessageListBoxOffset;
            auto* lbList = reinterpret_cast<CExoArrayList*>(
                reinterpret_cast<unsigned char*>(lb) +
                kListBoxControlsOffset);
            if (lbList && lbList->data) {
                int cap = rows > lbList->size ? lbList->size : rows;
                for (int i = s_lastComputerRows; i < cap; ++i) {
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
            // try again next tick
        }
    }
    if (rows < s_lastComputerRows) {
        acclog::Write("Dialog.Speech",
                      "computer rows reset %d -> %d",
                      s_lastComputerRows, rows);
    }
    s_lastComputerRows = rows;
}

// The main dialog panel: the NPC line, a terminal's output, and R to repeat
// the line.
void TickDialogPanel() {
    DialogPanelMatch m = FindActiveDialogPanel();
    static void*       s_lastPanel    = nullptr;
    static char        s_lastNpcLine[512] = {0};

    if (!m.panel) {
        if (s_lastPanel) {
            acclog::Write("Dialog.Speech", "panel closed");
            s_lastPanel = nullptr;
            s_lastNpcLine[0] = '\0';
            // A tutorial Q&A branch can record its hint and return straight
            // to the world with no further reply break, so the close fires
            // the pending popup. No-op when nothing is pending.
            acc::tutorial_popup::FirePendingAtReplyBreak();
        }
        return;
    }

    // First sight: a silent baseline. The first line arrives through the
    // change check once the engine fills the message label.
    if (s_lastPanel != m.panel) {
        s_lastPanel = m.panel;
        s_lastNpcLine[0] = '\0';
    }

    char npc[512] = "";
    bool gotNpc = acc::engine::ReadLabelTextAt(m.panel, kDialogMessageLabelOffset,
                                npc, sizeof(npc));
    if (gotNpc && std::strcmp(npc, s_lastNpcLine) != 0) {
        bool tutorialManaged = RecordTutorialLine(npc);
        SpeakerInfo info;
        bool resolved     = ResolveDialogSpeaker(info);
        bool suppressible = resolved && IsSuppressibleSpeaker(info);
        // Curated VO-less lines (Trask's Endar Spire level-up gate) would
        // otherwise be silenced entirely.
        bool forcedSpoken = acc::tutorial_hints::IsForcedSpokenDialogLine(npc);
        bool voicedLine = acc::tutorial_hints::IsVoicedSuppressedLine(npc);
        bool suppress = SilenceLine(suppressible, tutorialManaged, voicedLine,
                                    forcedSpoken);

        // A silenced line still gives its skill-check outcome marker.
        char marker[80] = "";
        bool spokeMarker = false;
        if (!suppress) {
            // A forced line queues behind the cue already in flight from the
            // same interaction ("This object is locked") instead of cutting
            // it off, and says once that R repeats it.
            acc::speech::Speak(npc, /*interrupt=*/!forcedSpoken);
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
        strncpy_s(s_lastNpcLine, npc, _TRUNCATE);
    }

    if (m.kind == acc::engine::PanelKind::DialogComputer &&
        acc::engine::IsListBox(
            reinterpret_cast<unsigned char*>(m.panel) +
            kDialogComputerMessageListBoxOffset)) {
        TickComputerRows(m.panel);
    }

    // R repeats the last line verbatim, suppressed or not: it is an explicit
    // request.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::DialogRepeatLine) &&
        s_lastNpcLine[0] != '\0') {
        acc::speech::Speak(s_lastNpcLine, /*interrupt=*/true);
        acclog::Write("Dialog.Speech", "R repeat -> [%.300s]", s_lastNpcLine);
    }
}

// A console's "view camera" swaps the terminal for
// CSWGuiDialogComputerCamera: a live picture whose one control, LBL_RETURN,
// holds the engine's own "press Enter to cancel the live feed and return".
// That label is the screen's whole accessible content, and it names the key
// the engine honours. Keyed on the camera panel's own presence, not on
// FindActiveDialogPanel: while a console is open the camera and conversation
// panels trade places in panels[] from tick to tick.
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
            acc::speech::Speak(acc::strings::Get(acc::strings::Id::CameraFeed),
                          /*interrupt=*/true);
            acclog::Write("Dialog.Speech",
                          "camera feed opened panel=%p — no readable label, "
                          "spoke the generic cue", cam);
        }
    }
}

// Bark bubbles, with their own lifecycle.
void TickBark() {
    void* bark = acc::engine::FindPanelByKind(acc::engine::PanelKind::BarkBubble);
    static void* s_lastBark = nullptr;
    static char  s_lastBarkText[512] = {0};
    if (!bark) {
        if (s_lastBark) {
            acclog::Write("Dialog.Speech", "bark closed");
            s_lastBark = nullptr;
            s_lastBarkText[0] = '\0';
            // A bark has no reply break; its close is when the VO is done, so
            // it fires a pending tutorial popup.
            acc::tutorial_popup::FirePendingAtReplyBreak();
        }
        return;
    }

    char text[512] = "";
    ReadFirstVisibleText(bark, text, sizeof(text));
    if (text[0] != '\0' && std::strcmp(text, s_lastBarkText) != 0) {
        bool tutorialManaged = RecordTutorialLine(text);
        SpeakerInfo info;
        bool resolved     = ResolveBarkSpeaker(bark, info);
        bool suppressible = resolved && IsSuppressibleSpeaker(info);
        bool voicedLine = acc::tutorial_hints::IsVoicedSuppressedLine(text);
        bool suppress = SilenceLine(suppressible, tutorialManaged, voicedLine,
                                    /*forcedSpoken=*/false);

        if (!suppress) {
            acc::speech::Speak(text, /*interrupt=*/false);
        }
        // A story-locked door or container explains itself once, in an
        // ownerless bark; locked_recall keeps it for the next locked attempt.
        acc::locked_recall::MaybeCapture(text, /*ownerless=*/!resolved);
        acclog::Write("Dialog.Speech",
                      "bark panel=%p speaker=[%s] appearance=%d race=%d "
                      "suppressible=%d tutorial=%d voiced=%d suppress=%d "
                      "-> [%.300s]",
                      bark, resolved ? info.tag : "?",
                      info.appearance, info.raceEnum,
                      suppressible ? 1 : 0, tutorialManaged ? 1 : 0,
                      voicedLine ? 1 : 0, suppress ? 1 : 0, text);
        strncpy_s(s_lastBarkText, text, _TRUNCATE);
    }
    s_lastBark = bark;
}

}  // namespace

void Tick() {
    // The engine tears panels[] down during a cutscene module load.
    if (acc::transitions::IsModuleLoadPending()) return;

    TickDialogPanel();
    TickCameraFeed();
    TickBark();
}

}  // namespace acc::dialog_speech
