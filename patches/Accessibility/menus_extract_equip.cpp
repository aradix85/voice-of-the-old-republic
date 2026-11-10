// The equipment screen's slot buttons: the slot name (TLK first, string
// table as fallback, the K2 weapon-set qualifier) and the equipped item.

#include "menus_extract_internal.h"
#include "engine_area.h"        // GetObjectDisplayNameByHandle
#include "engine_game.h"        // IsKotor2 — K1-only portraits.2da row table
#include "engine_panels.h"
#include "engine_player.h"      // PartyTableIsNPCAvailable / *Selectable /
#include "log.h"
#include "strings.h"
#include "engine_rebase.h"
#include "engine_offsets_select.h"

namespace acc::menus::extract {

namespace {

// 9b. Per-kind hardcoded label fallback for the equipment screen. The
//     BTN_INV_* slot buttons (and the matching LBL_INV_* labels) have
//     no inline text and no strref in equip.gui — same situation as the
//     InGameMenu strip icons. Prefer a dialog.tlk lookup so a non-German
//     install reads the engine's own translations, fall back to a
//     strings.h literal (which adapts to active language).
//
//     Slots are identified by INDEX in the engine's embedded
//     slot-button/label arrays (EquipSlotIndexFromControl — pointer
//     arithmetic against ctor-mined bases), NOT by .gui id: variant
//     equip_p.gui files renumber the ids per install
//     (userlogs/077noequipment; docs/gui-id-audit.md). The index order is
//     the slotInfo order both games' constructors share — weapL, weapR,
//     head, armL, armR, body, hands, implant, belt, and on KOTOR 2 the
//     appended weapL2, weapR2 — and the per-slot equipped-item handle is
//     the parallel array entry at kEquipPanelSlotItemIdsOffset + 4*index.
//     The TLK strrefs are the same in both games' dialog.tlk (verified
//     against the K2 TLK).
//
//     Both weapon sets carry a set qualifier on KOTOR 2, not just the
//     second one: the two rows are otherwise identical ("Left Weapon"
//     twice), and the panel itself labels the columns. The qualifier words
//     are the mod's own "active" / "secondary" (EquipSetActive /
//     EquipSetSecondary), NOT the engine's on-screen "Config 1" /
//     "Config 2" (dialog.tlk 49038/49039): the engine keeps no active-set
//     state — SwitchWeaps physically moves the items between the slot
//     pairs — so the first pair is by construction what the character
//     holds and the second pair the stored one. Numbered configs would
//     wrongly suggest a persistent "which set am I on".
struct EquipSlotName {
    uint32_t      strref;     // 0xFFFFFFFF = no TLK, use literal
    acc::strings::Id literalId;
    // Weapon-set qualifier, applied on KOTOR 2 only. 0 = no qualifier
    // (the non-weapon slots, shared between the two sets); 1 = active
    // pair, 2 = secondary pair.
    int           setQualifier;
};
// Indexed by engine slot index (see 9b note above). Entries 9/10 exist
// only on KOTOR 2; EquipSlotIndexFromControl never answers >= 9 on K1
// (EquipSlotCount bounds it).

const bool kEquipTableIsK2 = acc::game::IsKotor2();
const EquipSlotName k_equipSlotsByIndex[11] = {
    /* 0 weapL  */ { 31378u,      acc::strings::Id::EquipSlotWeapL,   1 },
    /* 1 weapR  */ { 31379u,      acc::strings::Id::EquipSlotWeapR,   1 },
    /* 2 head   */ { 31375u,      acc::strings::Id::EquipSlotHead,    0 },
    /* 3 armL   */ { 31376u,      acc::strings::Id::EquipSlotArmL,    0 },
    /* 4 armR   */ { 31377u,      acc::strings::Id::EquipSlotArmR,    0 },
    /* 5 body   */ { 31380u,      acc::strings::Id::EquipSlotBody,    0 },
    /* 6 hands  */ { 31383u,      acc::strings::Id::EquipSlotHands,   0 },
    /* 7 implant*/ { 0xFFFFFFFFu, acc::strings::Id::EquipSlotImplant, 0 },
    /* 8 belt   */ { 31382u,      acc::strings::Id::EquipSlotBelt,    0 },
    /* 9 weapL2 */ { 31378u,      acc::strings::Id::EquipSlotWeapL,   2 },
    /*10 weapR2 */ { 31379u,      acc::strings::Id::EquipSlotWeapR,   2 },
};

}  // namespace

const char* TryEquipSlot(void* control, void* owner,
                         char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::InGameEquip) {
        int idx = acc::menus::detail::EquipSlotIndexFromControl(owner,
                                                                control);
        if (idx >= 0) {
            const EquipSlotName& s = k_equipSlotsByIndex[idx];
            // Resolve the slot's localised label first into a small local
            // buffer; the equipped item name is then appended via the
            // FmtEquipSlot* templates so caller hears e.g. "Body, Combat
            // Suit" or "Body, empty".
            char slotLabel[128];
            slotLabel[0] = '\0';
            if (s.strref != 0xFFFFFFFFu) {
                char tlkText[128];
                if (LookupTlk(s.strref, tlkText, sizeof(tlkText))) {
                    size_t tlen = strnlen(tlkText, sizeof(tlkText));
                    if (tlen > 0 && tlen + 1 <= sizeof(slotLabel)) {
                        memcpy(slotLabel, tlkText, tlen + 1);
                    }
                }
            }
            if (slotLabel[0] == '\0') {
                const char* lit = acc::strings::Get(s.literalId);
                size_t llen = strlen(lit);
                if (llen > 0 && llen + 1 <= sizeof(slotLabel)) {
                    memcpy(slotLabel, lit, llen + 1);
                }
            }
            if (slotLabel[0] == '\0') return nullptr;

            // Weapon-set qualifier (KOTOR 2's two weapon rows). Appended to
            // the slot name so the pair reads "Left Weapon active" /
            // "Left Weapon secondary" instead of the same words twice.
            if (kEquipTableIsK2 && s.setQualifier != 0) {
                const char* setText = acc::strings::Get(
                    s.setQualifier == 1
                        ? acc::strings::Id::EquipSetActive
                        : acc::strings::Id::EquipSetSecondary);
                size_t slen = strnlen(slotLabel, sizeof(slotLabel));
                size_t qlen = strlen(setText);
                if (qlen > 0 && slen + 1 + qlen + 1 <= sizeof(slotLabel)) {
                    slotLabel[slen] = ' ';
                    memcpy(slotLabel + slen + 1, setText, qlen + 1);
                }
            }

            // Read the panel-cached slot handle (parallel array entry for
            // this slot index) and resolve to display name. The array is
            // populated by Equip's populate code from the displayed
            // character's CSWInventory (character cycling rewrites it),
            // so reading here always matches what's on screen.
            uint32_t handle = 0;
            if (acc::off::Ok(kEquipPanelSlotItemIdsOffset)) {
                __try {
                    handle = *reinterpret_cast<uint32_t*>(
                        reinterpret_cast<unsigned char*>(owner) +
                        kEquipPanelSlotItemIdsOffset +
                        4u * static_cast<size_t>(idx));
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    handle = 0;
                }
            }

            char itemName[128];
            itemName[0] = '\0';
            if (handle != 0 && handle != 0xffffffff) {
                if (!acc::engine::GetObjectDisplayNameByHandle(
                        handle, itemName, sizeof(itemName))) {
                    itemName[0] = '\0';
                }
            }

            using acc::strings::Get;
            using acc::strings::Id;
            if (itemName[0] != '\0') {
                snprintf(outBuf, bufSize, Get(Id::FmtEquipSlotItem),
                         slotLabel, itemName);
            } else {
                snprintf(outBuf, bufSize, Get(Id::FmtEquipSlotEmpty),
                         slotLabel);
            }
            source = "perkind-equip-slot";
            acclog::Write("Menus.PerKind",
                          "InGameEquip control=%p slot_index=%d slot=\"%s\" "
                          "handle=0x%x item=\"%s\" -> \"%s\"",
                          control, idx, slotLabel, handle, itemName, outBuf);
        }
    }
    return source;
}

bool ReadEquipSlotItemName(void* panel, void* slotBtn,
                           char* outBuf, size_t bufSize) {
    if (!panel || !slotBtn || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    int idx = acc::menus::detail::EquipSlotIndexFromButton(panel, slotBtn);
    if (idx < 0) return false;
    if (!acc::off::Ok(kEquipPanelSlotItemIdsOffset)) return false;
    uint32_t handle = 0;
    __try {
        handle = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(panel) +
            kEquipPanelSlotItemIdsOffset + 4u * static_cast<size_t>(idx));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    // 0x7f000000 = kInvalidObjectId, the engine's "slot empty" sentinel.
    if (handle == 0 || handle == 0xffffffff || handle == 0x7f000000)
        return false;
    return acc::engine::GetObjectDisplayNameByHandle(handle, outBuf,
                                                     bufSize);
}

}  // namespace acc::menus::extract
