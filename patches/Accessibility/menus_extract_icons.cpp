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
//     second one: the two rows are otherwise identical ("Linke Waffe"
//     twice), and the panel itself labels the columns. The qualifier words
//     are the mod's own "aktiv" / "sekund\xE4r" (EquipSetActive /
//     EquipSetSecondary), NOT the engine's on-screen "Konfig 1" /
//     "Konfig 2" (dialog.tlk 49038/49039): the engine keeps no active-set
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
            // Compose "{name}, im Team" / "{name}, verfügbar" so the
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

// 8. Speculative text read for known label/button vtable overrides.
//    Some classes override AsLabel/AsButton in their vtable so that
//    CallDowncast returns null even though the class IS label-like or
//    button-like at the field-offset level. The InGameMenu icons are
//    the canonical case: 8 sibling labels at vtable=0x0073E8E8 and
//    8 image-only buttons at vtable=0x0073E658, and our standard
//    extraction returns nullptr for all of them (panel-walk shows
//    src=none).
//
//    For each entry in kKnownVtableOverrides we try a direct read at
//    the standard label/button text offsets, guarded by SEH so that
//    reading at an offset that's NOT a CExoString doesn't crash the
//    game. If the structure is different, we'll get a SEH exception
//    and silently fall through to the placeholder path.
//
//    Allowlist gating keeps speculative reads off random unknown
//    vtables — we only fire on classes we've observed needing this.
const char* TrySpeculativeVtableRead(void* control, char* outBuf,
                                     size_t bufSize) {
    const char* source = nullptr;
    struct VtableOverrideInfo {
        uintptr_t vtable;
        bool      tryLabel;
        bool      tryButton;
        const char* tag;
    };
    static const VtableOverrideInfo k_knownOverrides[] = {
        // Sibling labels of in-game-menu icons (Equipment, Inventory,
        // Character, ...) — observed children [0..7] of CSWGuiInGameMenu.
        // Also chargen wizard step-number decorations.
        { 0x0073E8E8, true,  false, "label-spec" },
        // Image-only buttons (in-game-menu icons children [8..15],
        // chargen class icons, portrait-picker arrows).
        { 0x0073E658, false, true,  "button-spec" },
        // PartySelection portraits (vtable 0x00756BB8) are handled
        // in section 7b above — they store an NPC roster index, not
        // inline text, so the speculative offset reader never
        // resolves anything useful for them.
    };
    // Guarded like TryPartyPortrait's head: a walked control slot can hold a
    // non-null garbage pointer, and the vtable read is the first deref.
    void** vt = nullptr;
    __try {
        vt = *reinterpret_cast<void***>(control);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    uintptr_t vta = reinterpret_cast<uintptr_t>(vt);
    for (const auto& ov : k_knownOverrides) {
        // Table stays reference-build and compile-time: giving it a runtime
        // initialiser would make this function need object unwinding, which
        // its __try blocks below forbid (C2712). Map at the comparison.
        // R() returns 0 for a vtable with no KOTOR 2 mapping — without the
        // null guard, every garbage control whose first dword reads 0
        // "matches" both overrides and eats two SEH faults per visit (the
        // vtable=0x0 storm in userlogs/kennycrash0782408).
        uintptr_t ovMapped = acc::addr::R(ov.vtable);
        if (!ovMapped || ovMapped != vta) continue;
        char text[256];
        bool got = false;
        if (ov.tryLabel) {
            __try {
                // Path A: inline CExoString at standard label offset.
                if (ReadCExoString(control, kLabelTextOffset,
                                   text, sizeof(text))) {
                    got = true;
                }
                // Path B: strref at standard label offset → TLK.
                if (!got) {
                    uint32_t strref = ReadU32(control, kLabelStrRefOffset);
                    got = LookupTlk(strref, text, sizeof(text));
                }
                // Path C: text_object indirection. CSWGuiLabel.text.
                // text_params.text_object is a CSWGuiText* at +0x138; if
                // non-null, read its text_params.text (CExoString @+0x18)
                // or text_params.str_ref (@+0x20). Many labelhilights
                // route their rendered text through this pointer rather
                // than the inline CExoString.
                if (!got) {
                    void* textObj = *reinterpret_cast<void**>(
                        reinterpret_cast<unsigned char*>(control)
                        + kLabelTextObjectOffset);
                    if (textObj) {
                        if (ReadCExoString(textObj, kTextObjectTextOffset,
                                           text, sizeof(text))) {
                            got = true;
                        } else {
                            uint32_t strref = ReadU32(textObj,
                                                      kTextObjectStrRefOffset);
                            got = LookupTlk(strref, text, sizeof(text));
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                acclog::Write("Menus.SpecRead", "label SEH for vtable=0x%x "
                              "control=%p", (unsigned)vta, control);
                got = false;
            }
        }
        if (!got && ov.tryButton) {
            __try {
                // Path A: inline CExoString at standard button offset.
                if (ReadCExoString(control, kButtonTextOffset,
                                   text, sizeof(text))) {
                    got = true;
                }
                // Path B: strref at standard button offset → TLK.
                if (!got) {
                    uint32_t strref = ReadU32(control, kButtonStrRefOffset);
                    got = LookupTlk(strref, text, sizeof(text));
                }
                // Path C: text_object indirection at +0x1BC (button-side).
                if (!got) {
                    void* textObj = *reinterpret_cast<void**>(
                        reinterpret_cast<unsigned char*>(control)
                        + kButtonTextObjectOffset);
                    if (textObj) {
                        if (ReadCExoString(textObj, kTextObjectTextOffset,
                                           text, sizeof(text))) {
                            got = true;
                        } else {
                            uint32_t strref = ReadU32(textObj,
                                                      kTextObjectStrRefOffset);
                            got = LookupTlk(strref, text, sizeof(text));
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                acclog::Write("Menus.SpecRead", "button SEH for vtable=0x%x "
                              "control=%p", (unsigned)vta, control);
                got = false;
            }
        }
        if (got) {
            if (EmitText(outBuf, bufSize, text)) {
                source = ov.tag;
                // Trace: chain rebind / step / fingerprint all visit the
                // same control multiple times per arrow press; collapse
                // identical hit/empty/miss runs while preserving the
                // suppressed count.
                acclog::Trace("Menus.SpecRead",
                              "hit vtable=0x%x control=%p text=\"%s\"",
                              (unsigned)vta, control, outBuf);
                break;
            }
            acclog::Trace("Menus.SpecRead",
                          "empty vtable=0x%x control=%p "
                          "(read returned but text was empty)",
                          (unsigned)vta, control);
        } else {
            acclog::Trace("Menus.SpecRead",
                          "miss vtable=0x%x control=%p tag=%s",
                          (unsigned)vta, control, ov.tag);
        }
    }
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
        // Two K2 captions genuinely differ in wording: "Charaktere" (not
        // "Charakterblatt") and "Tagebuch" (not "Auftr\xe4ge").
        struct InGameMenuName {
            int         guiId;    // label id; the button is guiId + 8
            uint32_t    strrefK1; // 0xFFFFFFFF = no strref, use literal
            uint32_t    strrefK2;
            const char* literal;  // fallback if LookupTlk fails
        };
        static const InGameMenuName k_inGameMenuNames[8] = {
            // K2 has a standalone equipment strref where KOTOR 1 has none.
            { 0, 0xFFFFFFFFu, 48620u, "Ausr\xfcstung" },     // equipment
            { 1, 48220u,      48621u, "Inventar"      },     // inventory
            { 2, 48225u,      48622u, "Charakterblatt" },    // character_sheet
            { 3, 48224u,      48623u, "F\xe4higkeiten" },    // abilities
            // Slot 4 is messages on KOTOR 1 but the party screen on KOTOR 2:
            // its SwitchToSWInGameGui (0x007CA550) maps id 4 to the
            // PartySelection slot, and 48625 is K2's caption for it (48624 is
            // empty). The literal is the
            // KOTOR 1 lookup-failure fallback.
            { 4, 48223u,      48625u, "Nachrichten"   },     // messages / party
            { 5, 48218u,      48627u, "Auftr\xe4ge"   },     // journal
            { 6, 48221u,      48626u, "Karte"         },     // map
            { 7, 48222u,      48628u, "Optionen"      },     // options
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
            // the slot name so the pair reads "Linke Waffe aktiv" /
            // "Linke Waffe sekund\xE4r" instead of the same words twice.
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
