// Icon controls of the in-game screens: party portraits, the speculative
// read of an unknown control's text, the in-game menu icons, equipment
// slots and the map arrows.

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

// 7b. PartySelection portrait resolver (CSWGuiPartySelectionButton). The
//     CSWGuiPartySelection panel exposes 9 portrait slots (12 on KOTOR 2), one per
//     companion roster index. Each portrait is a
//     CSWGuiPartySelectionButton sub-classed from CSWGuiButton: the
//     standard text offsets carry nothing (set programmatically by
//     OnPanelAdded @0x006beeb0 with the creature's portrait image,
//     not a caption), so the speculative reader in section 8 below
//     returns empty and the user hears "control N".
//
//     Per-portrait layout: engine_player.h, kPartyPortrait*Offset (both
//     games, with witnesses); read through ReadPartyPortrait. The roster
//     index and the live selected flag are reliable on both games; the
//     KOTOR 1 flag word is not (uninitialised heap on some slots), so it is
//     trace-logged for diagnostics only.
//
//     Spoiler rule: only reveal the companion's name when the slot
//     is in the active party OR the engine's own
//     GetIsNPCAvailable returns true for that roster index. The
//     chain filter in RebindChain mirrors this so locked /
//     unavailable slots are dropped from arrow nav entirely.
//
//     `claimedVtable` reports whether the control IS a party portrait.
//     When it is and no text came out, the caller must stop the ladder
//     rather than fall through to the speculative reader at step 8 —
//     that reader never resolves anything useful for portraits.
const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize,
                             bool& claimedVtable) {
    const char* source = nullptr;
    claimedVtable = false;
    // IsPartyPortrait guards the vtable read: the walked control array can
    // hold a non-null GARBAGE entry (KOTOR 2 chargen name panel: an
    // unguarded read here crashed).
    if (!IsPartyPortrait(control)) return nullptr;
    claimedVtable = true;
    // `selected` is the LIVE flag every toggle flips — the ground truth as
    // the user adds/removes companions. partyId is only a panel-open
    // snapshot; reading it left the spoken status frozen at the opening
    // composition.
    PartyPortrait portrait;
    ReadPartyPortrait(control, &portrait);
    int npcSlot = portrait.npcSlot;
    int selected = portrait.selected;
    uint32_t flags = portrait.flags;
    int partyId = portrait.partyId;
    bool inActiveParty = selected != 0;
    bool available = PartyTableIsNPCAvailable(npcSlot);
    // Story locks: the engine refuses to toggle a companion whose
    // GetNPCSelectability is off — a benched one can't be added, a member
    // can't be removed. Both games' panels gate the click on it (K2 clears
    // the portrait's flags bit 0 and shows a message box), so say so up
    // front instead of calling the companion "available".
    bool selectable = PartyTableIsNPCSelectable(npcSlot);
    if ((inActiveParty || available) &&
        npcSlot >= 0 && npcSlot < PartyRosterSlotCount()) {
        char name[128];
        if (GetPartyNpcNameForSlot(npcSlot, name, sizeof(name)) &&
            name[0]) {
            // Compose "{name}, in party" / "{name}, available" so the
            // user hears whether the slot is currently selected for
            // the party or just on the bench, plus the lock when the
            // engine won't let it change. The per-tick focus
            // monitor re-runs this extractor and re-announces when
            // `selected` flips, so a toggle is heard immediately.
            // Unrecruited slots are dropped from the chain by
            // RebindChain and handled by the empty-slot branch below.
            acc::strings::Id statusFmt =
                inActiveParty
                    ? (selectable ? acc::strings::Id::FmtPartyPortraitInTeam
                                  : acc::strings::Id::FmtPartyPortraitInTeamLocked)
                    : (selectable ? acc::strings::Id::FmtPartyPortraitAvailable
                                  : acc::strings::Id::FmtPartyPortraitUnavailable);
            int n = _snprintf(outBuf,
                              bufSize,
                              acc::strings::Get(statusFmt),
                              name);
            if (n > 0 && static_cast<size_t>(n) < bufSize) {
                outBuf[n] = '\0';
                source = "party-portrait";
            } else {
                // Format overflowed — fall back to bare name so
                // the user still hears the companion.
                if (EmitText(outBuf, bufSize, name)) {
                    source = "party-portrait";
                }
            }
        }
    } else if (!inActiveParty && !available) {
        // Unrecruited / empty slot. The chain skips these, but the engine's
        // own Left/Right moves focus through the whole portrait grid and
        // lands on them, where the ladder had nothing better than
        // "control 15" (patch-20261029-192616.log). No name, per the
        // spoiler rule above.
        if (EmitText(outBuf, bufSize,
                     acc::strings::Get(acc::strings::Id::PartyPortraitEmpty))) {
            source = "party-portrait-empty";
        }
    }
    acclog::Trace("Menus.PartyPortrait",
                  "slot=%d selected=%d flags=0x%x partyId=%d "
                  "available=%d selectable=%d inActiveParty=%d text=\"%s\"",
                  npcSlot, selected, flags, partyId,
                  available ? 1 : 0, selectable ? 1 : 0,
                  inActiveParty ? 1 : 0, source ? outBuf : "");
    return source;
}

// 9a. Per-kind hardcoded label fallback. Some panels have widgets whose
//     text isn't extractable through any of the engine paths we know
//     (inline CExoString, strref, text_object, gui_string, tooltip all
//     verified empty for CSWGuiInGameMenu's icons in
//     patch-20260502-192712.log — 8 labels + 8 buttons all empty).
//     The text must be set via script-side or .gui-resource paths we
//     haven't traced yet. For known panel structures with fixed layout
//     (CSWGuiInGameMenu's struct definition pins the icon order), we
//     can hardcode the names by index until the engine-side path is
//     identified.
//
//     CSWGuiInGameMenu layout (per swkotor.exe.h:10145):
//       controls[0..7]  = 8 CSWGuiLabelHilight (vtable=0x0073E8E8)
//                          equipment, inventory, character, map,
//                          abilities, journal, options, messages
//       controls[8..15] = 8 CSWGuiButton       (vtable=0x0073E658)
//                          same names, same order
//
//     The user-visible captions in German are different from the
//     internal field names; the strings below are the rendered
//     in-game captions for the German build (matches "M" / "I" / "C"
//     hotkey conventions per the controls-and-input doc).
const char* TryInGameMenuIcon(void* control, void* owner,
                              char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner && IsPanelKindInGameMenu(owner)) {
        // Localized names sourced from dialog.tlk strrefs where they exist;
        // literal fallback for the one strref we couldn't find. Strref values
        // verified by parsing the user's actual dialog.tlk (German build,
        // LangID=2 — `tlk_lookup.ps1` results pasted into git history). The
        // strrefs are stable across localizations: the engine looks up the
        // SAME entry index from the locale-specific TLK, so a French or
        // English install gets correctly translated names without any per-
        // language switch. Equipment has no standalone strref in any
        // observed TLK — the engine never asks for that text via TLK — so
        // we fall back to a literal that matches the German build.
        // Keyed by the control's own .gui ID, not by its position in
        // controls[] — the same convention TryEquipSlot below uses, and the
        // reason this works unchanged on both games: top.gui (KOTOR 1) and
        // top_p.gui (KOTOR 2) assign the SAME ids (0..7 labels LBLH_*, 8..15
        // buttons BTN_*, equip/inv/char/abilities/messages/journal/map/
        // options), while the two engines build controls[] in different
        // member orders. Reading the id makes the array order irrelevant.
        // (Ids dumped with xoreos-tools gff2xml from each game's own gui.bif.)
        //
        // KOTOR 2 needs its own strrefs: its dialog.tlk assigns KOTOR 1's ids
        // to unrelated strings — 48218 reads "Ablativplatten Mk 1" and 48223 a
        // spacesuit hint there, which is exactly what the first KOTOR 2 test
        // round heard (patch-20260801-225529.log). The K2 cluster is
        // 48620..48628, mined the same way (parse TLK header, walk entries).
        struct InGameMenuName {
            int         guiId;    // label id; the button is guiId + 8
            uint32_t    strrefK1; // 0xFFFFFFFF = no strref, use literal
            uint32_t    strrefK2;
            const char* literal;  // fallback if LookupTlk fails
        };
        static const InGameMenuName k_inGameMenuNames[8] = {
            // K2 has a standalone equipment strref where KOTOR 1 has none.
            { 0, 0xFFFFFFFFu, 48620u, "Equipment" },
            { 1, 48220u,      48621u, "Inventory" },
            { 2, 48225u,      48622u, "Character" },
            { 3, 48224u,      48623u, "Abilities" },
            // Slot 4 is messages on KOTOR 1 but the party screen on KOTOR 2:
            // its SwitchToSWInGameGui (0x007CA550) maps id 4 to the
            // PartySelection slot, and 48625 is K2's caption for it (48624 is
            // empty). The literal is the
            // KOTOR 1 lookup-failure fallback.
            { 4, 48223u,      48625u, "Messages"  },     // messages / party
            { 5, 48218u,      48627u, "Journal" },
            { 6, 48221u,      48626u, "Map" },
            { 7, 48222u,      48628u, "Options" },
        };

        // Read the control's own .gui ID. Labels carry 0..7, their buttons
        // 8..15, so one table row serves both. The id is authoritative on
        // both games where the array position is not (see the table note).
        int cid = -1;
        __try {
            cid = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(control) + kControlIdOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            cid = -1;
        }
        int labelId = -1;
        if (cid >= 0 && cid <= 7)        labelId = cid;
        else if (cid >= 8 && cid <= 15)  labelId = cid - 8;
        if (labelId >= 0) {
            const InGameMenuName* found = nullptr;
            for (const auto& s : k_inGameMenuNames) {
                if (s.guiId == labelId) { found = &s; break; }
            }
            if (found) {
                const auto& spec = *found;
                const uint32_t strref = acc::game::IsKotor2() ? spec.strrefK2
                                                              : spec.strrefK1;
                bool gotTlk = false;
                if (strref != 0xFFFFFFFFu) {
                    char tlkText[256];
                    if (LookupTlk(strref, tlkText, sizeof(tlkText))) {
                        if (EmitText(outBuf, bufSize, tlkText)) {
                            source = "perkind-tlk";
                            gotTlk = true;
                            acclog::Write("Menus.PerKind", "InGameMenu TLK control=%p "
                                          "guiId=%d strref=%u -> \"%s\"",
                                          control, cid, strref, outBuf);
                        }
                    }
                }
                if (!gotTlk) {
                    // No-strref entries (Equipment: the engine never asks for
                    // its caption via TLK) can't reach the locale-stable
                    // strref path, so route them through the localised string
                    // table instead of the hardcoded German literal. The
                    // table literal stays as an ASCII last-ditch fallback.
                    const char* fallback = spec.literal;
                    if (strref == 0xFFFFFFFFu) {
                        fallback = acc::strings::Get(
                            acc::strings::Id::EquipMenuName);
                    }
                    if (EmitText(outBuf, bufSize, fallback)) {
                        source = "perkind-literal";
                        acclog::Write("Menus.PerKind", "InGameMenu literal control=%p "
                                      "guiId=%d strref=%u -> \"%s\"",
                                      control, cid, strref, outBuf);
                    }
                }
            }
        }
    }
    return source;
}

// 9b2. Per-kind label fallback for the in-game map (CSWGuiInGameMap).
//     The panel has two image-only buttons on either side of the map
//     render — `up_button` at struct offset +0xab0 and `down_button`
//     at +0xc74 (verified via the GoG xml MEMBER offsets +
//     Ghidra decomp of the panel's OnUpArrowPressed @0x006927b0 and
//     OnDownArrowPressed @0x006927c0 which both dispatch
//     CSWGuiPanel::HandleInputEvent(0x31/0x32, 1) → the InGameMap
//     override routes 0x31 → CSWGuiMapHider::GetPrevMapNote and
//     0x32 → CSWGuiMapHider::GetNextMapNote, cycling the engine's
//     filtered list of explored map-note waypoints).
//
//     Both buttons share vtable 0x0073E658 (CSWGuiButton) with empty
//     text and no strref — the speculative button-spec read at 9 hits
//     "miss" on both. Identify them by panel-base offset (analogous
//     to InGameEquip identifying slot buttons by .gui ID). The other
//     three buttons (return / partyselect / exit) have real text and
//     never reach this fallback.
const char* TryInGameMapArrow(void* control, void* owner,
                              char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner &&
        IdentifyPanel(owner) == PanelKind::InGameMap) {
        uintptr_t panelBase = reinterpret_cast<uintptr_t>(owner);
        uintptr_t ctrl      = reinterpret_cast<uintptr_t>(control);
        acc::strings::Id sid = acc::strings::Id::Count_;
        const char* tag = nullptr;
        if (ctrl == panelBase + kInGameMapUpButtonOffset) {
            sid = acc::strings::Id::MapPrevNote;
            tag = "MapPrevNote";
        } else if (ctrl == panelBase + kInGameMapDownButtonOffset) {
            sid = acc::strings::Id::MapNextNote;
            tag = "MapNextNote";
        }
        if (sid != acc::strings::Id::Count_) {
            const char* lit = acc::strings::Get(sid);
            if (EmitText(outBuf, bufSize, lit)) {
                source = "perkind-map";
                acclog::Write("Menus.PerKind", "InGameMap control=%p kind=%s -> \"%s\"",
                              control, tag, outBuf);
            }
        }
    }
    return source;
}

}  // namespace acc::menus::extract
