// The content monitor: a text fingerprint of each monitored panel, and speaking the segments that are new since the last tick.

#include "menus_monitors_internal.h"

#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus.h"            // SetDrilledIntoSubScreen — sole drill-arm site
#include "menus_chain.h"
#include "menus_extract.h"
#include "speech.h"
#include "tutorial_hints.h"  // TutorialBox keyboard-hint substitution (Surface 1)
#include "tutorial_popup.h"  // synthetic Trask-line tutorial popup (Surface 2)

namespace acc::menus::monitors {

namespace {

// ============================================================================
// Content fingerprint monitor.
// ============================================================================
struct ContentSnapshot {
    void* panel;
    char  text[8192];
};
constexpr int kMaxContentSnapshots = 8;
ContentSnapshot s_contentSnapshots[kMaxContentSnapshots];
int s_contentSnapshotCount = 0;

bool IsContentMonitored(PanelKind k) {
    switch (k) {
    case PanelKind::TutorialBox:
    // Dialog* panels and BarkBubble are owned by dialog_speech.cpp.
    // It reads message_label, replies count, computer-terminal lines,
    // and bark text — and gates the NPC + bark line speech through the
    // HumanSubtitles toggle. Letting this generic content fingerprint
    // also speak the dialog text caused a duplicate-speech race
    // (verified 2026-05-30 — user heard Carth's lines via this path
    // even after dialog_speech suppressed them).
    //
    // InGameMessages (combat log) and Container (loot panels) are owned
    // by the ListBoxPanelSpec paths in menus_listbox.cpp — each focused
    // row is announced once on Up/Down nav (verified live 2026-05-30
    // for Container). The fingerprint path was re-speaking the same
    // rows as generic controls; removing it eliminates the duplicate
    // without losing coverage.
    // InGameInventory and InGameEquip removed 2026-05-30 — the engine
    // populates the stat sidebar one tick AFTER the panel first appears
    // (placeholder header labels "VIT" / "DEF" / ... flip into real
    // values "6/6" / "15" / "Attack 0" etc.), which the fingerprint
    // diff path treated as new content and spoke as a batch on every
    // open. Per-row navigation inside the panels covers the rest.
    case PanelKind::MessageBoxModal:
    case PanelKind::AreaTransition:
    // StatusSummary is the engine's generic info popup (quest-progress and
    // journal-entry notices, skill-check results, "you received X"). Its body
    // is a cluster of per-notification labels plus a lone OK button, but it
    // was never content-monitored, so only the chain's OK button got read on
    // nav and the body text was silent (observed live 2026-06-03, StatusSummary
    // popup after entering Taris Southern Upper City). It has no
    // InGameSubScreenSpec, so first-sight speaks the body once. BuildContent
    // Fingerprint filters to the visible row(s) below — see that note.
    case PanelKind::StatusSummary:
    case PanelKind::InGameMap:
    // InGameJournal and InGameCharacter are tracked but SILENT — see
    // IsContentSpoken below for why they stay in this list.
    case PanelKind::InGameJournal:
    case PanelKind::InGameCharacter:
        return true;
    // InGameAbilities removed 2026-06-03: the dedicated handler in
    // menus_abilities.cpp owns every announcement on this screen now, so the
    // content fingerprint only double-spoke (the skill stats after a nav) and
    // re-spoke the new tab's first entry over the tab-name announce on a tab
    // switch. The screen-open title still comes from the sub-screen path.
    default:
        return false;
    }
}

// Of the content-monitored kinds, which may SPEAK their diff.
//
// The fingerprint serves two jobs: reading out new content, and detecting
// that a panel's content changed so a bound chain can rebind. A screen with
// its own keyboard navigation wants the second without the first — every
// row is already reachable and labelled, so speaking the diff on top only
// duplicates, or worse, dumps raw label text with no context.
//
// The monitored list has been shrinking that way for a while (Inventory +
// Equipment dropped 2026-05-30, Abilities 2026-06-03, Container and
// Messages before them). These two are the same case, but they cannot
// simply leave the list:
//
//   * InGameCharacter — a content change is what rebinds the chain when Tab
//     swaps the displayed party member and the Force-points row appears or
//     disappears (see the RebindChainPreserveIndex call in
//     MonitorPanelContents). Its twelve stat rows are arrow-reachable as
//     composed phrases ("Strength 14, +2"), whereas the fingerprint read the
//     value labels generically and spoke bare context-free numbers — "14",
//     "120000" — verified across the shipped logs.
//   * InGameJournal — menus_journal.cpp speaks each entry and its
//     description on navigation; the fingerprint re-spoke whole quest
//     bodies plus the entire entry list as one run-on line.
//
// InGameMap deliberately still speaks: the area name and map-note dump on
// open is genuinely useful orientation there rather than a duplicate.
bool IsContentSpoken(PanelKind k) {
    switch (k) {
    case PanelKind::InGameCharacter:
    case PanelKind::InGameJournal:
        return false;
    default:
        return true;
    }
}

void BuildContentFingerprint(void* panel, char* out, size_t outSize) {
    if (outSize == 0) return;
    out[0] = '\0';
    if (!panel) return;
    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return;
    int n = list->size > 32 ? 32 : list->size;
    PanelKind kind = IdentifyPanel(panel);
    size_t off = 0;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c) continue;
        if (CallDowncast(c, kVtableAsButton) != nullptr) continue;
        if (CallDowncast(c, kVtableAsButtonToggle) != nullptr) continue;
        if ((kind == PanelKind::Container ||
             kind == PanelKind::InGameEquip) && IsListBox(c)) continue;
        // StatusSummary lays out one label per notification type and shows
        // only the applicable row(s); hidden templates keep their "<CUSTOM0>"
        // placeholder text. Read only rows carrying the visible bit so speech
        // reflects what's on screen, not all eight templates.
        if (kind == PanelKind::StatusSummary) {
            uint32_t bitFlags = 0;  // guarded: row may be freed mid-walk
            if (!acc::engine::TryReadU32(c, kControlBitFlagsOffset, &bitFlags))
                continue;
            if ((bitFlags & kControlVisibleBit) == 0) continue;
        }

        char text[4096];
        const char* src = acc::menus::extract::FromControl(c, text, sizeof(text), panel);
        if (!src) continue;
        size_t tlen = strnlen(text, sizeof(text));
        if (tlen == 0) continue;

        bool allWs = true;
        for (size_t k = 0; k < tlen; ++k) {
            char ch = text[k];
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
                allWs = false; break;
            }
        }
        if (allWs) continue;

        size_t needed = (off > 0 ? 3 : 0) + tlen;
        if (off + needed + 1 >= outSize) break;
        if (off > 0) {
            out[off++] = ' ';
            out[off++] = '|';
            out[off++] = ' ';
        }
        memcpy(out + off, text, tlen);
        off += tlen;
        out[off] = '\0';
    }
}

bool FingerprintContainsSegment(const char* hay, size_t hayLen,
                                const char* seg, size_t segLen) {
    if (segLen == 0 || segLen > hayLen) return false;
    const char* sep = " | ";
    const size_t sepLen = 3;
    size_t i = 0;
    while (i + segLen <= hayLen) {
        if (memcmp(hay + i, seg, segLen) == 0) {
            bool startOk = (i == 0) ||
                (i >= sepLen && memcmp(hay + i - sepLen, sep, sepLen) == 0);
            bool endOk = (i + segLen == hayLen) ||
                (i + segLen + sepLen <= hayLen &&
                 memcmp(hay + i + segLen, sep, sepLen) == 0);
            if (startOk && endOk) return true;
        }
        ++i;
    }
    return false;
}

void SpeakNewSegments(const char* prev, const char* curr) {
    const char* sep = " | ";
    const size_t sepLen = 3;
    size_t prevLen = strlen(prev);
    const char* p = curr;
    while (*p) {
        const char* end = strstr(p, sep);
        size_t segLen = end ? (size_t)(end - p) : strlen(p);
        if (segLen > 0 &&
            !FingerprintContainsSegment(prev, prevLen, p, segLen)) {
            char seg[8192];
            size_t cp = segLen < sizeof(seg) - 1 ? segLen : sizeof(seg) - 1;
            memcpy(seg, p, cp);
            seg[cp] = '\0';
            // This monitor owns no dedup channel, and on a message box it
            // competes with the row hook: the box's prompt is both the panel's
            // new content AND its single active row, so both paths spoke it
            // and confirm prompts were read out twice. Marking after speaking
            // closes the other direction too — whichever focus announce runs
            // next won't repeat what was just said here.
            if (acc::menus::AlreadySpokenOnAnyChannel(seg)) {
                acclog::Write("ContentChange",
                    "  already spoken by a focus/row announce — skipped \"%s\"",
                    seg);
            } else {
                acc::speech::Speak(seg, /*interrupt=*/false);
                acc::menus::MarkSpoken(/*channel=*/0, seg);
                acclog::Write("ContentChange", "  spoke \"%s\"", seg);
            }
        }
        if (!end) break;
        p = end + sepLen;
    }
}

char* GetContentSnapshot(void* panel) {
    for (int i = 0; i < s_contentSnapshotCount; ++i) {
        if (s_contentSnapshots[i].panel == panel) {
            return s_contentSnapshots[i].text;
        }
    }
    if (s_contentSnapshotCount >= kMaxContentSnapshots) {
        memmove(s_contentSnapshots, s_contentSnapshots + 1,
                sizeof(s_contentSnapshots[0]) * (kMaxContentSnapshots - 1));
        s_contentSnapshotCount = kMaxContentSnapshots - 1;
    }
    int idx = s_contentSnapshotCount++;
    s_contentSnapshots[idx].panel = panel;
    s_contentSnapshots[idx].text[0] = '\0';
    return s_contentSnapshots[idx].text;
}

}  // namespace

void MonitorPanelContents() {
    constexpr int kCap = 32;
    void* panelData[kCap];
    int panelCount = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panelData, kCap);
    if (panelCount <= 0) return;

    AnnounceNewSubScreens(panelData, panelCount);

    for (int i = 0; i < panelCount; ++i) {
        void* p = panelData[i];
        if (!p) continue;
        PanelKind k = IdentifyPanel(p);
        if (!IsContentMonitored(k)) continue;

        char fingerprint[8192];
        BuildContentFingerprint(p, fingerprint, sizeof(fingerprint));

        // Tutorial popups: the vanilla message is worded for mouse. Leave the
        // on-screen text alone (sighted / rest-sight users still read it) and
        // speak a keyboard-oriented line instead. Keyed by the popup's source
        // tutorial.2da row (+0x994). Overriding the fingerprint with a constant
        // hint makes SpeakNewSegments say it once on first sight and stay quiet
        // as the engine pages through multi-message tutorials. Non-mapped rows
        // (no mouse wording) fall through to the vanilla rendered text. The
        // engine-focus mouse announce (AnnounceNewFocusedControl) and the
        // arrow-nav chain entry are separately gated off for these rows in
        // menus.cpp / menus_chain.cpp so this is the sole speaker.
        if (k == PanelKind::TutorialBox) {
            // Synthetic Trask-line popup: speak its keyboard hint regardless of
            // the (repurposed) row. Otherwise fall back to the row-keyed hint.
            const char* hint = acc::tutorial_popup::SyntheticActive()
                                   ? acc::tutorial_popup::SyntheticHint()
                                   : acc::tutorial_hints::HintForTutorialRow(
                                         acc::tutorial_hints::ReadTutorialRow(p));
            // Row-keyed lookup misses on KOTOR 2 (every mapped popup there
            // spoke its mouse wording in the field logs), so fall back to the
            // strref-keyed one — the same lookup the row hook in menus.cpp
            // uses. Both paths must resolve the SAME string: the row hook is
            // the primary speaker for these popups and this monitor only
            // repeats what it said, which the announce dedup then collapses.
            // Resolving different strings here would double-announce.
            if (!hint) hint = acc::tutorial_hints::HintForMouseText(fingerprint);
            if (hint) strncpy_s(fingerprint, sizeof(fingerprint), hint, _TRUNCATE);
        }

        char* last = GetContentSnapshot(p);
        if (strncmp(last, fingerprint, sizeof(s_contentSnapshots[0].text)) == 0) {
            continue;
        }

        bool firstSight = (last[0] == '\0');
        bool suppressFirstSight =
            firstSight &&
            (k == PanelKind::Container || FindSpec(k) != nullptr);
        if (suppressFirstSight) {
            strncpy_s(last, sizeof(s_contentSnapshots[0].text),
                      fingerprint, _TRUNCATE);
            acclog::Write("ContentChange", "panel=%p kind=%s first-sight snapshot "
                          "(deferring to kind-name path): \"%.200s\"",
                          p, PanelKindName(k), fingerprint);
            continue;
        }

        if (fingerprint[0] != '\0') {
            acclog::Write("ContentChange", "panel=%p kind=%s",
                          p, PanelKindName(k));
            acclog::Write("ContentChange", "  prev=\"%.300s\"", last);
            acclog::Write("ContentChange", "  curr=\"%.300s\"", fingerprint);
            if (IsContentSpoken(k)) {
                SpeakNewSegments(last, fingerprint);
            } else {
                acclog::Write("ContentChange",
                              "  silent kind — tracked for chain rebind only");
            }

            // If the chain is bound to this panel, an engine-driven content
            // change can alter which virtual rows belong in the chain — the
            // character sheet's Force-points row appears/disappears with the
            // displayed character's Jedi-ness on Tab (leader switch). The
            // panel pointer is unchanged across a Tab, so HandleNavStep's
            // "activePanel != g_chainPanel" rebind never fires and the row
            // set goes stale. Rebuild here (preserving the cursor) so the
            // chain tracks live content, matching how the rest of the sheet
            // updates dynamically on Tab.
            if (acc::menus::chain::g_chainPanel == p) {
                acc::menus::chain::RebindChainPreserveIndex(p);
            }
        } else {
            acclog::Write("ContentChange", "panel=%p kind=%s fingerprint cleared "
                          "(prev=\"%.100s\")", p, PanelKindName(k), last);
        }
        strncpy_s(last, sizeof(s_contentSnapshots[0].text),
                  fingerprint, _TRUNCATE);
    }
}

}  // namespace acc::menus::monitors
