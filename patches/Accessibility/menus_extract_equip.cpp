// The equipment screen's slot buttons: the slot name (TLK first, the string
// table as fallback, the K2 weapon-set qualifier) and the equipped item.

#include "menus_extract_internal.h"

#include "engine_area.h"     // GetObjectDisplayNameByHandle
#include "engine_game.h"     // IsKotor2
#include "engine_offsets.h"  // kInvalidObjectId
#include "engine_panels.h"
#include "log.h"
#include "strings.h"

namespace acc::menus::extract {

namespace {

// The BTN_INV_* slot buttons carry no text and no strref in equip.gui.
// Slots are known by INDEX in the engine's slot-button arrays
// (EquipSlotIndexFromControl), not by .gui id: variant equip_p.gui files
// renumber the ids. Index order is both constructors' slotInfo order, and
// the equipped item's handle is the parallel entry at
// kEquipPanelSlotItemIdsOffset + 4 * index. The strrefs are the same in both
// games' dialog.tlk.
//
// K2's two weapon pairs read "active" / "secondary" (the mod's words, not
// the engine's "Config 1" / "Config 2"): the engine keeps no active-set
// state, SwitchWeaps moves the items between the slot pairs, so the first
// pair is what the character holds.
struct EquipSlotName {
    uint32_t         strref;
    acc::strings::Id literalId;
    int              setQualifier;  // K2: 0 none, 1 active pair, 2 secondary pair
};

// Entries 9 and 10 exist only on K2 (EquipSlotCount bounds the index on K1).
const EquipSlotName k_equipSlotsByIndex[11] = {
    /* 0 weapL  */ { 31378u,    acc::strings::Id::EquipSlotWeapL,   1 },
    /* 1 weapR  */ { 31379u,    acc::strings::Id::EquipSlotWeapR,   1 },
    /* 2 head   */ { 31375u,    acc::strings::Id::EquipSlotHead,    0 },
    /* 3 armL   */ { 31376u,    acc::strings::Id::EquipSlotArmL,    0 },
    /* 4 armR   */ { 31377u,    acc::strings::Id::EquipSlotArmR,    0 },
    /* 5 body   */ { 31380u,    acc::strings::Id::EquipSlotBody,    0 },
    /* 6 hands  */ { 31383u,    acc::strings::Id::EquipSlotHands,   0 },
    /* 7 implant*/ { kNoStrref, acc::strings::Id::EquipSlotImplant, 0 },
    /* 8 belt   */ { 31382u,    acc::strings::Id::EquipSlotBelt,    0 },
    /* 9 weapL2 */ { 31378u,    acc::strings::Id::EquipSlotWeapL,   2 },
    /*10 weapR2 */ { 31379u,    acc::strings::Id::EquipSlotWeapR,   2 },
};

bool SlotLabel(int idx, char* out, size_t size) {
    const EquipSlotName& s = k_equipSlotsByIndex[idx];
    out[0] = '\0';
    if (s.strref == kNoStrref || !LookupTlk(s.strref, out, size)) {
        strncpy_s(out, size, acc::strings::Get(s.literalId), _TRUNCATE);
    }
    if (!out[0]) return false;
    if (acc::game::IsKotor2() && s.setQualifier != 0) {
        using acc::strings::Id;
        const char* q =
            acc::strings::Get(s.setQualifier == 1 ? Id::EquipSetActive : Id::EquipSetSecondary);
        const size_t len = strnlen(out, size);
        const size_t qlen = strlen(q);
        if (qlen > 0 && len + 1 + qlen + 1 <= size) {
            out[len] = ' ';
            memcpy(out + len + 1, q, qlen + 1);
        }
    }
    return true;
}

// The panel's per-slot handles are filled from the displayed character's
// inventory (character cycling rewrites them), so they match the screen.
bool SlotItemName(void* panel, int idx, char* out, size_t size) {
    out[0] = '\0';
    uint32_t handle = 0;
    if (!acc::off::Ok(kEquipPanelSlotItemIdsOffset) ||
        !TryReadU32(panel, kEquipPanelSlotItemIdsOffset + 4u * static_cast<size_t>(idx), &handle) ||
        handle == 0 || handle == 0xffffffff || handle == kInvalidObjectId) {
        return false;
    }
    if (acc::engine::GetObjectDisplayNameByHandle(handle, out, size)) return true;
    out[0] = '\0';
    return false;
}

}  // namespace

const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize) {
    if (!owner || IdentifyPanel(owner) != PanelKind::InGameEquip) return nullptr;
    const int idx = acc::menus::detail::EquipSlotIndexFromControl(owner, control);
    if (idx < 0) return nullptr;
    char slotLabel[128];
    if (!SlotLabel(idx, slotLabel, sizeof(slotLabel))) return nullptr;
    char itemName[128];
    using acc::strings::Get;
    using acc::strings::Id;
    if (SlotItemName(owner, idx, itemName, sizeof(itemName))) {
        snprintf(outBuf, bufSize, Get(Id::FmtEquipSlotItem), slotLabel, itemName);
    } else {
        snprintf(outBuf, bufSize, Get(Id::FmtEquipSlotEmpty), slotLabel);
    }
    acclog::Write("Menus.PerKind", "InGameEquip control=%p slot_index=%d slot=\"%s\" "
                  "item=\"%s\" -> \"%s\"", control, idx, slotLabel, itemName, outBuf);
    return "perkind-equip-slot";
}

bool ReadEquipSlotItemName(void* panel, void* slotBtn, char* outBuf, size_t bufSize) {
    if (!panel || !slotBtn || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    const int idx = acc::menus::detail::EquipSlotIndexFromButton(panel, slotBtn);
    return idx >= 0 && SlotItemName(panel, idx, outBuf, bufSize);
}

}  // namespace acc::menus::extract
