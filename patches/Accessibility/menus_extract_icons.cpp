// Image-only controls of the in-game screens, named from what they stand
// for: the party portraits, the in-game menu icons and the map's note
// arrows.

#include "menus_extract_internal.h"

#include "engine_game.h"     // IsKotor2
#include "engine_offsets_select.h"
#include "engine_panels.h"
#include "engine_player.h"   // PartyTableIsNPCAvailable / Selectable, ReadPartyPortrait
#include "log.h"
#include "strings.h"

namespace acc::menus::extract {

namespace {

constexpr uint32_t kNoStrref = 0xFFFFFFFFu;

// The in-game menu's icons, keyed by the control's own .gui id: top.gui
// (K1) and top_p.gui (K2) give the 8 labels ids 0..7 and their buttons
// 8..15 in the same order, while the two engines fill controls[] in
// different orders. K2's dialog.tlk puts unrelated strings at K1's strrefs,
// so it has its own (48620..48628).
struct InGameMenuName {
    int         guiId;     // label id; the button is guiId + 8
    uint32_t    strrefK1;
    uint32_t    strrefK2;
    const char* literal;   // if the lookup fails
};

constexpr InGameMenuName kInGameMenuNames[8] = {
    // K1 has no equipment strref (the engine never asks for that caption).
    { 0, kNoStrref, 48620u, "Equipment" },
    { 1, 48220u,    48621u, "Inventory" },
    { 2, 48225u,    48622u, "Character" },
    { 3, 48224u,    48623u, "Abilities" },
    // Messages on K1, the party screen on K2: its SwitchToSWInGameGui
    // (0x007CA550) maps id 4 to PartySelection (48624 is empty).
    { 4, 48223u,    48625u, "Messages"  },
    { 5, 48218u,    48627u, "Journal"   },
    { 6, 48221u,    48626u, "Map"       },
    { 7, 48222u,    48628u, "Options"   },
};

}  // namespace

// CSWGuiPartySelectionButton: OnPanelAdded (0x006beeb0) gives each portrait
// the companion's image and no caption, so it is read through
// ReadPartyPortrait (engine_player.h). The roster index and the live
// selected flag are reliable in both games; K1's flag word is not, and is
// only traced. Spoiler rule: the name only for a party member or a
// companion GetIsNPCAvailable admits (RebindChain drops the rest from the
// chain). claimedVtable says the control IS a portrait, so the ladder stops
// here even without text.
const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize,
                             bool& claimedVtable) {
    claimedVtable = false;
    // IsPartyPortrait guards the vtable read: a walked control array can
    // hold a non-null garbage entry (an unguarded read crashed on K2's
    // chargen name panel).
    if (!IsPartyPortrait(control)) return nullptr;
    claimedVtable = true;
    PartyPortrait portrait;
    ReadPartyPortrait(control, &portrait);
    const int npcSlot = portrait.npcSlot;
    // `selected` is the live flag every toggle flips; partyId is a snapshot
    // from when the panel opened.
    const bool inActiveParty = portrait.selected != 0;
    const bool available = PartyTableIsNPCAvailable(npcSlot);
    // A story lock: the engine refuses to toggle a companion whose
    // selectability is off (K2 shows a message box), so it is said up front.
    const bool selectable = PartyTableIsNPCSelectable(npcSlot);

    const char* source = nullptr;
    if ((inActiveParty || available) && npcSlot >= 0 && npcSlot < PartyRosterSlotCount()) {
        char name[128];
        if (GetPartyNpcNameForSlot(npcSlot, name, sizeof(name)) && name[0]) {
            // The focus monitor re-runs this and re-announces when
            // `selected` flips, so a toggle is heard at once.
            using acc::strings::Id;
            const Id fmt = inActiveParty
                               ? (selectable ? Id::FmtPartyPortraitInTeam
                                             : Id::FmtPartyPortraitInTeamLocked)
                               : (selectable ? Id::FmtPartyPortraitAvailable
                                             : Id::FmtPartyPortraitUnavailable);
            const int n = _snprintf(outBuf, bufSize, acc::strings::Get(fmt), name);
            if (n > 0 && static_cast<size_t>(n) < bufSize) {
                outBuf[n] = '\0';
                source = "party-portrait";
            } else if (EmitText(outBuf, bufSize, name)) {
                source = "party-portrait";  // the bare name when the format overflows
            }
        }
    } else if (!inActiveParty && !available) {
        // An unrecruited or empty slot: the chain skips these, but the
        // engine's own Left/Right walks the whole grid and lands on them.
        if (EmitText(outBuf, bufSize, acc::strings::Get(acc::strings::Id::PartyPortraitEmpty))) {
            source = "party-portrait-empty";
        }
    }
    acclog::Trace("Menus.PartyPortrait",
                  "slot=%d selected=%d flags=0x%x partyId=%d "
                  "available=%d selectable=%d inActiveParty=%d text=\"%s\"",
                  npcSlot, portrait.selected, portrait.flags, portrait.partyId,
                  available ? 1 : 0, selectable ? 1 : 0,
                  inActiveParty ? 1 : 0, source ? outBuf : "");
    return source;
}

// No engine path carries the in-game menu icons' text (inline, strref,
// text_object, gui_string and tooltip are all empty), so they are named from
// the table by .gui id.
const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize) {
    if (!owner || !IsPanelKindInGameMenu(owner)) return nullptr;
    const int cid = ReadControlId(control);
    if (cid < 0 || cid > 15) return nullptr;
    const InGameMenuName& spec = kInGameMenuNames[cid % 8];
    const uint32_t strref = acc::game::IsKotor2() ? spec.strrefK2 : spec.strrefK1;
    char tlkText[256];
    if (strref != kNoStrref && LookupTlk(strref, tlkText, sizeof(tlkText)) &&
        EmitText(outBuf, bufSize, tlkText)) {
        acclog::Write("Menus.PerKind", "InGameMenu TLK control=%p guiId=%u strref=%u -> \"%s\"",
                      control, cid, strref, outBuf);
        return "perkind-tlk";
    }
    // K1's equipment icon has no strref: the string table names it.
    const char* fallback = (strref == kNoStrref)
                               ? acc::strings::Get(acc::strings::Id::EquipMenuName)
                               : spec.literal;
    if (!EmitText(outBuf, bufSize, fallback)) return nullptr;
    acclog::Write("Menus.PerKind", "InGameMenu literal control=%p guiId=%u strref=%u -> \"%s\"",
                  control, cid, strref, outBuf);
    return "perkind-literal";
}

// CSWGuiInGameMap's two image-only buttons beside the map (up_button,
// down_button): HandleInputEvent 0x31 / 0x32, which the map routes to
// CSWGuiMapHider::GetPrevMapNote / GetNextMapNote, cycling the explored map
// notes. Both share CSWGuiButton's vtable with no text, so they are known
// by their offset in the panel.
const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize) {
    if (!owner || IdentifyPanel(owner) != PanelKind::InGameMap) return nullptr;
    const uintptr_t panelBase = reinterpret_cast<uintptr_t>(owner);
    const uintptr_t ctrl = reinterpret_cast<uintptr_t>(control);
    acc::strings::Id sid;
    if (ctrl == panelBase + kInGameMapUpButtonOffset) {
        sid = acc::strings::Id::MapPrevNote;
    } else if (ctrl == panelBase + kInGameMapDownButtonOffset) {
        sid = acc::strings::Id::MapNextNote;
    } else {
        return nullptr;
    }
    if (!EmitText(outBuf, bufSize, acc::strings::Get(sid))) return nullptr;
    acclog::Write("Menus.PerKind", "InGameMap control=%p kind=%s -> \"%s\"", control,
                  sid == acc::strings::Id::MapPrevNote ? "MapPrevNote" : "MapNextNote", outBuf);
    return "perkind-map";
}

}  // namespace acc::menus::extract
