// Rows that only exist in the mod's model of a screen (virtual rows): the
// character sheet stats, credits, equipment stats, pazaak deck and wager,
// the journal's quest-items button and key-binding rows.

#include "menus_extract_internal.h"

namespace acc::menus::extract {

namespace {

// ---- Pazaak wager popup virtual row --------------------------------------
// CSWGuiWagerPopup.field13_0xc94 is the live wager (UpdateWagerText writes it
// to wager_value_label each step). The maximum_label renders
// "Maximaler Einsatz: M\nCredits: K". We surface a single top-of-chain row
// combining the live wager with that max+credits line — the analogue of the
// inventory credits row.
// KOTOR 2: +0xe30, observed in its own CSWGuiWagerPopup::HandleInputEvent
// (vtable slot 15), which uses it exactly as KOTOR 1's does — hands it to the
// wager-exit call, decrements it under `if (1 < value)`, and increments it under
// `if (value < max)` where max is the next field along (0xe34, KOTOR 1 0xc98).
const size_t kWagerCurrentValueOffset = acc::off::Pick(0xc94, 0xe30);

// LBL_MAXIMUM / BTN_LESS / BTN_MORE all resolve through the popup's own
// ctor-bound members (WagerPopupPanel* in menus_internal.cpp); the two games'
// .gui ids for them survive only as those resolvers' GuiIdMismatch tripwires.
void* FindWagerMaxLabel(void* panel) {
    return acc::menus::detail::WagerPopupPanelMaximumLabel(panel);
}

// Format "Einsatz N. Maximaler Einsatz: M, Credits: K" from the live wager
// (panel field) + the maximum_label's rendered text (`maxLabel`).
bool ExtractWagerRow(void* panel, void* maxLabel, char* outBuf, size_t bufSize) {
    if (!panel || !maxLabel || bufSize == 0) return false;
    int wager = -1;
    __try {
        wager = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(panel) + kWagerCurrentValueOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    if (wager < 0) return false;

    char raw[96];
    raw[0] = '\0';
    __try {
        if (!acc::engine::ReadGuiString(maxLabel, kLabelGuiStringPtrOffset,
                                        raw, sizeof(raw))) {
            ExtractTextOrStrRefIndirect(maxLabel, kLabelTextOffset,
                                        kLabelStrRefOffset, kLabelTextObjectOffset,
                                        raw, sizeof(raw));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    if (raw[0] == '\0') return false;

    // Flatten the embedded newline so the screen reader reads one line.
    char flat[128];
    size_t fi = 0;
    for (const char* p = raw; *p && fi + 2 < sizeof(flat); ++p) {
        if (*p == '\n' || *p == '\r') { flat[fi++] = ','; flat[fi++] = ' '; }
        else                          { flat[fi++] = *p; }
    }
    flat[fi] = '\0';

    snprintf(outBuf, bufSize,
             acc::strings::Get(acc::strings::Id::PazaakFmtWagerRow), wager, flat);
    return outBuf[0] != '\0';
}

// Character-sheet stat rows on InGameCharacter.
const char* TryCharsheetStatRow(void* control, void* owner,
                                char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner && IdentifyPanel(owner) ==
            PanelKind::InGameCharacter &&
        acc::menus::charsheet::IsStatRowAnchor(owner, control)) {
        __try {
            if (acc::menus::charsheet::ExtractStatRow(
                    owner, control, outBuf, bufSize) &&
                outBuf[0] != '\0') {
                source = "perkind-charsheet-row";
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            source = nullptr;
        }
    }
    return source;
}

// Credits row (Inventory + Store). IsCreditsRowAnchor self-gates on
// the owning panel's kind, so the per-kind branch isn't repeated
// here — same shape as the stat-row block. Only fires when the
// control is the panel's credits_value_label inline member.
const char* TryCreditsRow(void* control, void* owner,
                          char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        acc::menus::credits::IsCreditsRowAnchor(owner, control)) {
        __try {
            if (acc::menus::credits::ExtractCreditsRow(
                    owner, control, outBuf, bufSize) &&
                outBuf[0] != '\0') {
                source = "perkind-credits-row";
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            source = nullptr;
        }
    }
    return source;
}

// Equip stat rows (Vitality, Defense, Attack, Damage). Same
// shape as credits — IsEquipStatRowAnchor self-gates on
// InGameEquip; ExtractEquipStatRow handles single-vs-dual-wield
// attack formatting internally.
const char* TryEquipStatRow(void* control, void* owner,
                            char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        acc::menus::equipstats::IsEquipStatRowAnchor(owner, control)) {
        __try {
            if (acc::menus::equipstats::ExtractEquipStatRow(
                    owner, control, outBuf, bufSize) &&
                outBuf[0] != '\0') {
                source = "perkind-equipstat-row";
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            source = nullptr;
        }
    }
    return source;
}

// Pazaak side-deck builder card widgets (CSWGuiPazaakStart). The
// widgets carry no text of their own; synthesize the card name +
// owned count (available grid) or the chosen-slot contents (the
// already-selected state).
const char* TryPazaakDeckCard(void* control, void* owner,
                              char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::PazaakStart) {
        __try {
            if (acc::menus::pazaakdeck::ExtractCardLabel(
                    owner, control, outBuf, bufSize) &&
                outBuf[0] != '\0') {
                source = "perkind-pazaakdeck";
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            source = nullptr;
        }
    }
    return source;
}

// Pazaak wager popup (CSWGuiWagerPopup). BTN_LESS / BTN_MORE are
// text-less CSWGuiSpeedButtons; name them so the chain doesn't fall back to
// "control N". The live amount is announced separately by pazaak.cpp (the
// chain re-reads only on focus change).
const char* TryPazaakWager(void* control, void* owner,
                           char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::PazaakWager) {
        const bool isLess =
            control == acc::menus::detail::WagerPopupPanelLessButton(owner);
        const bool isMore =
            control == acc::menus::detail::WagerPopupPanelMoreButton(owner);
        if (isLess || isMore) {
            snprintf(outBuf, bufSize, "%s",
                     acc::strings::Get(isLess
                         ? acc::strings::Id::PazaakWagerLess
                         : acc::strings::Id::PazaakWagerMore));
            if (outBuf[0] != '\0') source = "perkind-pazaakwager";
        } else if (control ==
                       acc::menus::detail::WagerPopupPanelMaximumLabel(owner) &&
                   ExtractWagerRow(owner, control, outBuf, bufSize)) {
            // Virtual top-of-chain row: live wager + max + credits.
            source = "perkind-pazaakwager-row";
        }
    }
    return source;
}

// Journal quest-items button. The engine renders an unclear German
// label ("Aus Auftrag"); speak a clearer localized term on focus.
// Identified by struct offset (quest_items_button @ +0x8a4). The
// sub-screen it opens still announces its own LBL_TITLE on entry.
const char* TryJournalQuestItemsButton(void* control, void* owner,
                                       char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::InGameJournal &&
        control == reinterpret_cast<unsigned char*>(owner) +
                       kJournalQuestItemsButtonOffset) {
        snprintf(outBuf, bufSize, "%s",
                 acc::strings::Get(
                     acc::strings::Id::JournalQuestItemsButton));
        if (outBuf[0] != '\0') source = "perkind-journal-questitems-btn";
    }
    return source;
}

// Keyboard-mapping row (CSWGuiKeyMapButton). The row's own button text
// (action_button at +0) is just the event name ("Vorwärts"); the bound
// key lives in the embedded mapped_key_button at +0x1c8. Compose
// "{action}: {key}" so the user hears the current binding, and flag
// non-remappable rows. Gated on the owning panel kind AND the row
// vtable so the panel's other buttons (filter tabs, OK/Abbrechen) fall
// through to the normal ladder.
const char* TryKeyBindingRow(void* control, void* owner,
                             char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::KeyboardMapping) {
        __try {
            uintptr_t vt = reinterpret_cast<uintptr_t>(
                *reinterpret_cast<void***>(control));
            if (vt == kVtableKeyMapButton) {
                char action[128]; action[0] = '\0';
                char key[64];     key[0] = '\0';
                acc::engine::ReadButtonText(control, action, sizeof(action));
                acc::engine::ReadButtonText(
                    reinterpret_cast<unsigned char*>(control) +
                        kKeyMapButtonMappedKeyOffset,
                    key, sizeof(key));
                int fixed = *reinterpret_cast<int*>(
                    reinterpret_cast<unsigned char*>(control) +
                        kKeyMapButtonUnchangeableOff);
                snprintf(outBuf, bufSize,
                         acc::strings::Get(acc::strings::Id::FmtKeyBinding),
                         action, key);
                if (fixed != 0) {
                    size_t cur = strnlen(outBuf, bufSize);
                    const char* suf = acc::strings::Get(
                        acc::strings::Id::KeyBindingFixed);
                    snprintf(outBuf + cur, bufSize - cur, "%s", suf);
                }
                if (outBuf[0] != '\0') source = "perkind-keybinding";
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            source = nullptr;
        }
    }
    return source;
}

}  // namespace

// 0. Per-kind row formatter for virtual chain entries. Runs BEFORE
//    the standard text-extraction ladder because the formatted
//    phrase must OVERRIDE the bare label text. lbl_class's inline
//    CExoString is "Soldat"; the user navigating the stat chain
//    expects "Klasse: Soldat" with the context word, so without
//    this early hook the AsLabel path (section 4) would return the
//    bare value and never reach the per-kind handler.
//
//    Only fires when `control` is a registered anchor for the
//    owning panel's kind (IsStatRowAnchor short-circuits otherwise),
//    so non-virtual controls in InGameCharacter still hit the
//    standard ladder.
//
//    Owner is resolved here rather than reusing step 10 resolution: that
//    one runs after the standard extract sections and we need the owner
//    now. Cost is one extra FindOwningPanel call when ownerPanel is null
//    (announce-control path).
const char* TryVirtualRowAnchor(void* control, void* ownerPanel,
                                char* outBuf, size_t bufSize) {
    void* owner = ResolveOwnerPanel(control, ownerPanel);
    const char* source = nullptr;
    if (!source) source = TryCharsheetStatRow(control, owner, outBuf, bufSize);
    if (!source) source = TryCreditsRow(control, owner, outBuf, bufSize);
    if (!source) source = TryEquipStatRow(control, owner, outBuf, bufSize);
    if (!source) source = TryPazaakDeckCard(control, owner, outBuf, bufSize);
    if (!source) source = TryPazaakWager(control, owner, outBuf, bufSize);
    if (!source) source = TryJournalQuestItemsButton(control, owner, outBuf, bufSize);
    if (!source) source = TryKeyBindingRow(control, owner, outBuf, bufSize);
    return source;
}

void ForEachWagerRowAnchor(void* panel,
                           bool (*callback)(void* labelControl, int sortCy,
                                            void* userData),
                           void* userData) {
    if (!panel || !callback) return;
    if (IdentifyPanel(panel) != acc::engine::PanelKind::PazaakWager) return;
    void* maxLabel = FindWagerMaxLabel(panel);
    if (!maxLabel) return;
    // sortCy=1 lands the wager row at the very top of the chain, above the
    // less/more/Setzen/Beenden buttons (cy 226+) — same placement as credits.
    callback(maxLabel, /*sortCy=*/1, userData);
}

}  // namespace acc::menus::extract
