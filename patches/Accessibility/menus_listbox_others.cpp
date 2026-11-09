// Two small list screens: the examine view and the party AI script
// selection.

#include "menus_listbox_internal.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// ScriptSelect — the character sheet's "Kurzbefehle" combat-behaviour picker
// (CSWGuiScriptSelect). A single ai_state_list_box (id 0) holds 3
// CSWGuiButtonToggle rows (the AI presets). It is single-select: the panel's
// confirm handler reads GetSelectedControl(&ai_state_list_box) and writes the
// one chosen row's value into creature_stats->ai_state.
//
// Engine semantics (decompiled CSWGuiScriptSelect::HandleInputEvent): the
// activate code 0x27 (our Enter) IS the "Wählen"/confirm action — it applies
// the selected row and pops+destroys the panel; 0x28 (Esc) is "Abbrechen"
// (close without applying). There is no per-row toggle action, so we drive it
// as a select-then-confirm listbox: Up/Down move the listbox selection (which
// is what Wählen reads), Enter routes to BTN_WAEHLEN, Esc to BTN_ABBRECHEN.
//
// The toggle rows render a localized on/off suffix ("Standardangriff, ein").
// That suffix tracks the committed state at panel open, not the pending
// selection (DriveListBoxSelection bypasses the engine's onSelectionChanged
// redraw — same caveat as SaveLoad's preview labels), so the announce strips
// it and speaks just the behaviour name + position.
// ============================================================================
// Option table (CExoArrayList data pointer at panel+0x64). The constructor
// fills it from aiscripts.2da, one 8-byte entry per row indexed by the row's
// control id: +0 = DESCRIPTION_STRREF, +4 = AISTATE. (Decompiled
// CSWGuiScriptSelect ctor @0x006ea000 / OnScriptSelected @0x006e9e70.)
// Kotor1Only rather than Todo: KOTOR 2 ships no CSWGuiScriptSelect at all
// (see engine_panels.cpp), so there is no K2 value to go and find.
const size_t kScriptSelectOptionTableOffset = acc::off::Kotor1Only(0x64);
constexpr size_t kScriptSelectOptionStride      = 8;
constexpr size_t kScriptSelectOptionDescOffset  = 0;

// PowersLevelUp (pwrlvlup.gui) is NOT a flat listbox despite the
// "powers_listbox" name in the SARIF struct. Its rows are CSWGuiSkillFlow
// tree-rows with up to 3 cells per row (base / improved / master variants),
// identical in shape to the chargen Talente chart. A ListBoxPanelSpec can't
// represent that 2D structure — Up/Down would only change row, not column,
// and the row text via FromControl returns empty because the row IS a
// CSWGuiSkillFlow with no rendered label. Handled in menus_powers_levelup.cpp
// with the chargen_feats-style 2D dispatcher; title override moved to the
// AnnouncePanelTitle path in menus.cpp.

// ============================================================================
// Examine — CSWGuiExamine panel opened by Ö. Listbox is the embedded
// CSWGuiMessageBox.listbox_message at +0x67c. The engine populates the
// rows from a local object cache when ShowExamineBox(handle, 0) is called
// (vtable[27] on the listbox does the populate-from-object — verified
// 2026-05-22 from the ShowExamineBox decomp). Up/Down speak each row;
// Enter / Esc let the engine's HandleInputEvent handle close natively
// (Schliess. and Abbrechen buttons both call HideExamineBox).
// ============================================================================

bool ExamineMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::Examine;
}

void* ExamineFindLb(void* p) {
    // See InGameMessagesFindLb.
    return acc::off::Ptr(p, kExaminePanelListBoxOffset);
}

// The `rowCount <= 0` guard is this spec's own, and InGameMessagesAnnounce
// deliberately does not have it. Whether that is a real difference is
// doubtful — a non-null r.row with a rowCount of 0 should not occur — but
// it is a behaviour question, not a cleanup one, so the consolidation kept
// each site's guard exactly as it found it.
void ExamineAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (r.rowCount <= 0) return;
    SpeakRowAndPosition(r);
}

bool ScriptSelectMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::ScriptSelect;
}

void* ScriptSelectFindLb(void* p) {
    return ScriptSelectPanelAiStateListBox(p);
}

// Speak the behaviour name + position, dropping the toggle's ", ein"/", aus"
// state suffix. Strip at the last ", " only when the trailing token is a
// single word (no space) — the localized on/off markers always are, while a
// future behaviour name containing ", " (with multi-word tail) is left intact.
void ScriptSelectAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (!r.row || r.rowCount <= 0) return;
    char rowText[256];
    if (!acc::menus::extract::FromControl(r.row, rowText, sizeof(rowText))) {
        return;
    }
    char* lastSep = nullptr;
    for (char* c = rowText; *c; ++c) {
        if (c[0] == ',' && c[1] == ' ') lastSep = c;
    }
    if (lastSep) {
        bool tailIsToken = true;
        for (char* c = lastSep + 2; *c; ++c) {
            if (*c == ' ') { tailIsToken = false; break; }
        }
        if (tailIsToken) *lastSep = '\0';
    }
    char msg[320];
    snprintf(msg, sizeof(msg),
             acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
             rowText, r.newSel + 1, r.rowCount);
    prism::Speak(msg, /*interrupt=*/false);
}

// Per-row enrichment: speak the behaviour's description. The engine only
// refreshes the LB_DESC listbox through the row's hover callback (which
// DriveListBoxSelection bypasses), so instead of mutating that listbox we
// resolve the row's DESCRIPTION_STRREF from the panel's option table directly
// and speak it. Index by the row's control id — the same index OnScriptSelected
// uses to read AISTATE — not the listbox position.
void ScriptSelectEnrichRow(void* panel, const ListBoxNavResult& r) {
    if (!panel || !r.row) return;
    __try {
        int id = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(r.row) + kControlIdOffset);
        if (id < 0) return;
        void* table = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(panel) +
            kScriptSelectOptionTableOffset);
        if (!table) return;
        uint32_t descStrref = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(table) +
            id * kScriptSelectOptionStride + kScriptSelectOptionDescOffset);
        char desc[1024];
        if (LookupTlk(descStrref, desc, sizeof(desc)) && desc[0] != '\0') {
            prism::Speak(desc, /*interrupt=*/false);
            acclog::Write("ScriptSelect",
                          "desc id=%d strref=%u (first 80: \"%.80s\")",
                          id, descStrref, desc);
        } else {
            acclog::Write("ScriptSelect",
                          "desc id=%d strref=%u empty/unresolved", id, descStrref);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("ScriptSelect",
                      "enrichRow faulted panel=%p row=%p", panel, r.row);
    }
}

// Enter → Wählen: applies the selected ai_state and closes. Esc → Abbrechen:
// closes without applying. Both are plain CSWGuiButtons; activating them
// drives the panel's confirm/cancel path (engine codes 0x2d / 0x2e), which
// reads the same selection_index DriveListBoxSelection writes.
bool ScriptSelectOnEnter(void* panel) {
    QueueControlActivate(ScriptSelectPanelAcceptButton(panel),
                         "ScriptSelect: Enter -> BTN_WAEHLEN");
    return true;
}

bool ScriptSelectOnEsc(void* panel) {
    QueueControlActivate(ScriptSelectPanelBackButton(panel),
                         "ScriptSelect: Esc -> BTN_ABBRECHEN");
    return true;
}

}  // namespace

constexpr ListBoxPanelSpec kExamineSpec = {
    /*logTag*/                  "Examine",
    /*matches*/                 ExamineMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             ExamineFindLb,
    /*minSel*/                  0,
    /*announce*/                ExamineAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                nullptr,
    /*onEnter*/                 nullptr,  // engine HandleInputEvent closes via OK button
    /*onEsc*/                   nullptr,  // engine HandleInputEvent closes via Cancel button
    /*titleOverride*/           nullptr,
    /*emptyStateId*/            acc::strings::Id::Count_,
    /*alwaysReturnFromHandler*/ false,
};

constexpr ListBoxPanelSpec kScriptSelectSpec = {
    /*logTag*/                  "ScriptSelect",
    /*matches*/                 ScriptSelectMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             ScriptSelectFindLb,
    /*minSel*/                  0,
    /*announce*/                ScriptSelectAnnounce,
    /*enrichRow*/               ScriptSelectEnrichRow,
    /*logExtra*/                nullptr,
    /*onEnter*/                 ScriptSelectOnEnter,
    /*onEsc*/                   ScriptSelectOnEsc,
    /*titleOverride*/           nullptr,             // real "Kurzbefehl-Auswahl" label
    /*emptyStateId*/            acc::strings::Id::Count_,  // always 3 presets
    /*alwaysReturnFromHandler*/ true,                // self-contained modal popup
};

}  // namespace acc::menus::listbox
