// The workbench screen: its upgrade slots and the create-items button.

#include "menus_extract_internal.h"

#include "engine_area.h"        // GetObjectDisplayNameByHandle
#include "engine_game.h"        // IsKotor2 — K1-only portraits.2da row table
#include "engine_panels.h"
#include "log.h"
#include "strings.h"

namespace acc::menus::extract {

namespace {

// 9b3. Per-kind label for workbench upgrade-panel slot buttons
//     (upgrade.gui IDs 12..18). The buttons have empty inline text;
//     their visual content is the installed mod's icon + name set
//     programmatically. The engine carries the SLOT TYPE NAME
//     (Energiezelle / Vibrationszelle / Sch\xE4rfe / …) in a 16-entry
//     strref table at 0x00756fb0, indexed by
//     `(slot_btn.custom_value - 4) + panel.field25_0x2f4c * 4`.
//     Read it dynamically so the user hears the actual slot type
//     they're focused on — matching the LBL_SLOTNAME the engine
//     would render for sighted users when they hover.
//
//     The category enum (field25_0x2f4c) selects the table column:
//         1 = lightsaber (saber UX with crystal picker)
//         2 = 4-slot non-saber (crystal slot widgets)
//         3 = 3-slot non-saber weapon (Aufwertungs widgets)
//         4 = 2-slot non-saber (crystal widgets, slots 1..2)
//     Sentinel entries (UpgradeType = -1) mark positions the
//     category doesn't use; fall back to the position-only name for
//     those so the user still gets *some* orientation.
// Position-only slot names, keyed by the embedded slot-button array index
// (crystal bank first, then weapon bank — construction order in both
// games' ctors). Namespace scope because TryWorkbenchSlot is SEH-heavy
// (MSVC C2712 bars function-local statics there).
const acc::strings::Id k_workbenchSlotNameByIndexK1[7] = {
    acc::strings::Id::WorkbenchSlotSaberCrystal1,
    acc::strings::Id::WorkbenchSlotSaberCrystal2,
    acc::strings::Id::WorkbenchSlotSaberCrystal3,
    acc::strings::Id::WorkbenchSlotSaberCrystal4,
    acc::strings::Id::WorkbenchSlotWeapon1,
    acc::strings::Id::WorkbenchSlotWeapon2,
    acc::strings::Id::WorkbenchSlotWeapon3,
};
const acc::strings::Id k_workbenchSlotNameByIndexK2[9] = {
    acc::strings::Id::WorkbenchSlotSaberCrystal1,
    acc::strings::Id::WorkbenchSlotSaberCrystal2,
    acc::strings::Id::WorkbenchSlotSaberCrystal3,
    acc::strings::Id::WorkbenchSlotSaberCrystal4,
    acc::strings::Id::WorkbenchSlotSaberCrystal5,
    acc::strings::Id::WorkbenchSlotSaberCrystal6,
    acc::strings::Id::WorkbenchSlotWeapon1,
    acc::strings::Id::WorkbenchSlotWeapon2,
    acc::strings::Id::WorkbenchSlotWeapon3,
};

}  // namespace

const char* TryWorkbenchSlot(void* control, void* owner,
                             char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::WorkbenchUpgrade) {
        // Matched by position in the panel's embedded slot-button run —
        // variant .gui files renumber the ids (docs/gui-id-audit.md).
        int slotArrayIdx =
            acc::menus::detail::UpgradeSlotIndexFromButton(owner, control);
        if (slotArrayIdx >= 0) {
            // The two diagnostic lines below (slot-type classification +
            // occupancy state) are re-emitted every frame by the focused-
            // control monitor while the user dwells on a slot. They share the
            // "Menus.PerKind" tag and INTERLEAVE (A,B,A,B…), which defeats
            // Trace's line-level dedup (each line differs from the one just
            // before it). BlockLog would be the usual fold for interleaved
            // lines, but this function uses __try/__except and MSVC forbids an
            // object with a destructor there (C2712). So instead we split the
            // two lines onto SEPARATE tags: each tag then sees only its own
            // line repeated identically during a dwell (A,A,A… / B,B,B…) and
            // Trace folds each to "(repeated Nx more)". No information lost —
            // any change (new slot, new mod) flushes and re-prints in full.
            uint8_t category = 0;
            int customValue = -1;
            __try {
                category = *(reinterpret_cast<unsigned char*>(owner)
                             + kUpgradePanelCategoryOff);
                customValue = *reinterpret_cast<int*>(
                    reinterpret_cast<unsigned char*>(control)
                    + kUpgradeSlotCustomValueOff);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                category = 0;
                customValue = -1;
            }
            // Per-game index + table read live in the engine layer; see
            // LookupUpgradeSlotType for why they cannot be inlined here.
            int upgradeType = -1;
            uint32_t strref = 0;
            bool haveEntry = acc::engine::LookupUpgradeSlotType(
                customValue, (int)category, &upgradeType, &strref);
            bool resolved = false;
            if (haveEntry) {
                if (acc::engine::LookupTlk(strref, outBuf, bufSize)) {
                    source = "perkind-workbench-slot";
                    resolved = outBuf[0] != '\0';
                }
            }
            if (resolved) {
                acclog::Trace("Menus.PerKind",
                              "WorkbenchUpgrade control=%p slot_index=%d "
                              "cat=%d cval=%d type=%d strref=%u -> \"%s\"",
                              control, slotArrayIdx, (int)category,
                              customValue, upgradeType, strref, outBuf);
            } else {
                // Fallback: position-only name. Used when the table
                // entry is a sentinel (slot doesn't apply to this
                // category) or strref lookup fails.
                //
                // Keyed by the embedded-array index: the constructors
                // build the crystal bank first, then the weapon bank, in
                // slot-number order in both games (K1: crystals 1..4 then
                // weapons 1..3; K2: the six-slot saber bank then weapons
                // 1..3), so the index maps positionally — no .gui ids.
                // Tables at namespace scope (k_workbenchSlotNameByIndex*):
                // this function is SEH-heavy and MSVC C2712 bars locals
                // needing init guards.
                acc::strings::Id sid = acc::strings::Id::Count_;
                if (acc::game::IsKotor2()) {
                    if (slotArrayIdx < 9)
                        sid = k_workbenchSlotNameByIndexK2[slotArrayIdx];
                } else {
                    if (slotArrayIdx < 7)
                        sid = k_workbenchSlotNameByIndexK1[slotArrayIdx];
                }
                if (sid != acc::strings::Id::Count_) {
                    const char* lit = acc::strings::Get(sid);
                    if (EmitText(outBuf, bufSize, lit)) {
                        source = "perkind-workbench-slot";
                        acclog::Trace("Menus.PerKind",
                                      "WorkbenchUpgrade control=%p slot_index=%d "
                                      "cat=%d cval=%d type=%d strref=%u "
                                      "(sentinel/empty) -> fallback \"%s\"",
                                      control, slotArrayIdx, (int)category,
                                      customValue, upgradeType, strref, outBuf);
                    }
                }
            }

            // Append occupancy state so the user hears whether the slot
            // already holds an upgrade — "Power Cell, holds Massive Criticals"
            // when occupied, "Power Cell, empty" when empty. Reuses the
            // shared equip-slot composition templates. field35_0x2f74[cval]
            // is the installed mod item (engine-constructed from the mod
            // template at panel-open / install; see OnPanelAdded).
            if (source) {
                char slotName[160];
                size_t snlen = strnlen(outBuf, bufSize);
                if (snlen + 1 <= sizeof(slotName)) {
                    memcpy(slotName, outBuf, snlen + 1);
                    using acc::strings::Get;
                    using acc::strings::Id;
                    void* installed = acc::engine::GetWorkbenchSlotInstalledItem(
                        owner, control);
                    if (installed) {
                        char modName[160];
                        modName[0] = '\0';
                        bool gotName = acc::engine::ExtractTextOrStrRef(
                                           installed, kItemLocNameOffset,
                                           kItemLocNameOffset + 4, modName,
                                           sizeof(modName)) &&
                                       modName[0] != '\0';
                        if (gotName) {
                            // "<slot type>, holds <mod>" — connecting word
                            // so an identical slot/mod name doesn't read twice.
                            snprintf(outBuf, bufSize,
                                     Get(Id::WorkbenchFmtSlotItem),
                                     slotName, modName);
                        } else {
                            // Name unresolved: "<slot type>, occupied".
                            snprintf(outBuf, bufSize, Get(Id::FmtEquipSlotItem),
                                     slotName, Get(Id::WorkbenchSlotFilled));
                        }
                    } else {
                        snprintf(outBuf, bufSize, Get(Id::FmtEquipSlotEmpty),
                                 slotName);
                    }
                    acclog::Trace("Menus.PerKind.SlotState",
                                  "WorkbenchUpgrade slot state control=%p "
                                  "installed=%p -> \"%s\"",
                                  control, installed, outBuf);
                }
            }
        }
    }
    return source;
}

// 9b4. Per-kind label for the K2 workbench top screen's BTN_CREATEITEMS
//      (upgradesel_p.gui id 7). The .gui bakes no strref and no text and
//      the ctor never SetTexts it (icon-only for sighted users), so the
//      generic ladder falls through to the "control 7" placeholder. Speak
//      the game's own TLK line 111574 ("Create Items" — the
//      title of the screen the button opens), which localizes for free.
//      K1's upgradesel has no id-7 button-with-empty-text (id 7 is a
//      ProtoItem icon there and never chain-navigable), but the check is
//      gated per game anyway.
const char* TryWorkbenchCreateItemsButton(void* control, void* owner,
                                          char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner && acc::game::IsKotor2() &&
        IdentifyPanel(owner) == PanelKind::WorkbenchSelect) {
        int cid = -1;
        __try {
            cid = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(control) + kControlIdOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            cid = -1;
        }
        if (cid == 7) {
            constexpr uint32_t kCreateItemsTitleStrRef = 111574;
            if (acc::engine::LookupTlk(kCreateItemsTitleStrRef, outBuf,
                                       bufSize) &&
                outBuf[0] != '\0') {
                source = "perkind-workbench-createitems";
                acclog::Trace("Menus.PerKind",
                              "WorkbenchSelect BTN_CREATEITEMS control=%p "
                              "strref=%u -> \"%s\"",
                              control, kCreateItemsTitleStrRef, outBuf);
            }
        }
    }
    return source;
}

}  // namespace acc::menus::extract
