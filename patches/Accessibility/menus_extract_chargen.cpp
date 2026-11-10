// Character creation: the class selection icons and the portrait arrows
// (with the portrait variants in portraits.2da order).

#include "menus_extract_internal.h"

#include "engine_game.h"        // IsKotor2 — K1-only portraits.2da row table
#include "engine_panels.h"
#include "log.h"
#include "strfmt.h"   // AsciiLower
#include "strings.h"

namespace acc::menus::extract {

namespace {

// CSWCCreature::GetPortraitId thiscall typedef — verified moving in the
// chargen probe (24 → 25 across a net +1 cycle). Storage location is
// opaque; the accessor is the only reliable source.
typedef uint32_t (__thiscall* PFN_CSWCCreatureGetPortraitId)(void*);

// CSWCCreature::GetPortrait — fills caller-supplied 16-byte CResRef.
// `side` is 0 for the default (light-side) baseresref. The function
// returns the same outBuf pointer; we don't use the return value.
typedef void* (__thiscall* PFN_CSWCCreatureGetPortrait)(
    void* this_, void* outBuf, int side);

// The chargen-selectable rows of each game's portraits.2da, in row order —
// which IS the order the engine's Left/Right cycle visits them (confirmed in
// patch-20260803-102731.log: cycling male portraits on KOTOR 2 walked ids
// 60, 45, 32, 31, 30 … 18 and wrapped, exactly the forpc=1 && sex=0 set with
// the forpc=0 row 22 skipped).
//
// We keep only what the announce needs: the row id, the gender letter and
// the head-family letter of the baseresref stem (`po_p[mf]h[abch]NN`). The
// point of the table is the VARIANT NUMBER. The stem's own digits are not a
// position — KOTOR 2's five light-skinned male portraits are rows 23..27
// spelling 01, 06, 03, 04, 07, and its five dark-skinned males are 06, 08,
// 10, 07, 09 — so announcing the digit made the cycle sound shuffled, which
// is what the user reported. Ranking the row inside its (gender, family)
// group turns those into 1..5 in the order the user actually walks them.
// On KOTOR 1 the digits already equal the ranks, so its wording is
// unchanged.
//
// A row the table doesn't know (portrait mods) falls back to the stem's
// digits at the use site, so a modded install degrades to the old wording
// rather than to a wrong number.
struct PortraitRow {
    unsigned char row;     // portraits.2da row == CSWCCreature::GetPortraitId
    char          gender;  // 'm' | 'f'
    char          family;  // 'a' | 'b' | 'c' | 'h'  (K2 only ships 'h')
};

// KOTOR 1: rows 1..12 and 15..32 (13 = po_pt3m3 and 14 = po_pcarth are
// forpc=0). Transcribed from build/2da-extracted/portraits.2da.
constexpr PortraitRow kPortraitRowsK1[] = {
    { 1,'f','a'}, { 2,'f','a'}, { 3,'f','a'}, { 4,'f','a'}, { 5,'f','a'},
    { 6,'f','c'}, { 7,'f','c'}, { 8,'f','c'}, { 9,'f','c'}, {10,'f','c'},
    {11,'f','b'}, {12,'f','b'}, {15,'f','b'}, {16,'f','b'}, {17,'f','b'},
    {18,'m','a'}, {19,'m','a'}, {20,'m','a'}, {21,'m','a'}, {22,'m','a'},
    {23,'m','c'}, {24,'m','c'}, {25,'m','c'}, {26,'m','c'}, {27,'m','c'},
    {28,'m','b'}, {29,'m','b'}, {30,'m','b'}, {31,'m','b'}, {32,'m','b'},
};

// KOTOR 2: same shape plus the two TSL-only head families (P_MAL_H /
// P_FEM_H, rows 45/60 and 58/59), and one fewer Asian male (row 22
// po_PMHA03 is forpc=0). Transcribed from
// build/k2-2da-extracted/portraits.2da.
constexpr PortraitRow kPortraitRowsK2[] = {
    { 1,'f','a'}, { 2,'f','a'}, { 3,'f','a'}, { 4,'f','a'}, { 5,'f','a'},
    { 6,'f','c'}, { 7,'f','c'}, { 8,'f','c'}, { 9,'f','c'}, {10,'f','c'},
    {11,'f','b'}, {12,'f','b'}, {15,'f','b'}, {16,'f','b'}, {17,'f','b'},
    {18,'m','a'}, {19,'m','a'}, {20,'m','a'}, {21,'m','a'},
    {23,'m','c'}, {24,'m','c'}, {25,'m','c'}, {26,'m','c'}, {27,'m','c'},
    {28,'m','b'}, {29,'m','b'}, {30,'m','b'}, {31,'m','b'}, {32,'m','b'},
    {45,'m','h'}, {58,'f','h'}, {59,'f','h'}, {60,'m','h'},
};

using acc::strfmt::AsciiLower;  // KOTOR 2's portraits.2da mixes case

// 1-based position of `row` inside its own (gender, family) group, or 0 when
// the row isn't one this game's table knows.
int PortraitVariantRank(uint32_t row) {
    const PortraitRow* table = acc::game::IsKotor2() ? kPortraitRowsK2
                                                     : kPortraitRowsK1;
    int count = acc::game::IsKotor2()
                    ? (int)(sizeof(kPortraitRowsK2) / sizeof(kPortraitRowsK2[0]))
                    : (int)(sizeof(kPortraitRowsK1) / sizeof(kPortraitRowsK1[0]));
    int self = -1;
    for (int i = 0; i < count; ++i) {
        if (table[i].row == row) { self = i; break; }
    }
    if (self < 0) return 0;
    int rank = 1;
    for (int i = 0; i < self; ++i) {
        if (table[i].gender == table[self].gender &&
            table[i].family == table[self].family) {
            ++rank;
        }
    }
    return rank;
}

}  // namespace

// 9c. Per-kind label override for the chargen class-selection panel
//     (CSWGuiClassSelection, vtable=0x00758020). Panel hosts 6
//     image-only class-icon buttons (vtable=0x0073E658, empty text)
//     in class_selections[6]; the engine maintains the currently-
//     hovered class name in the parent's class_label, which it
//     updates via OnEnterButton.
//
//     Two timing hazards we work around:
//
//     1. STALE READ on focus event. SetActiveControl fires before the
//        engine's OnEnterButton has updated class_label, so reading
//        class_label at chain-step / SetActive time returns the
//        PREVIOUSLY focused icon's class. The chain step's
//        AnnounceControl would then speak "previous icon's class" —
//        an audible echo of the prior selection.
//
//     2. ECHO REVERT. After the engine briefly updates class_label
//        for the user's target icon, an internal bounce / re-select
//        chain (engine sets active_control back to a different
//        "selected" icon) reverts class_label to that other icon's
//        class. The per-frame focused-control monitor catches the
//        revert and speaks it as a phantom "echo of an earlier
//        navigated entry."
//
//     The fix: cache (icon → class_text) lazily — only populate when
//     active_control == icon AND class_label is non-empty AND the
//     nav-suppress budget has settled. Once cached, lock the entry:
//     subsequent extracts return the cached text regardless of the
//     engine's transient class_label state, so the revert can't fire
//     a phantom utterance. Combine with chain-step gating in the
//     navigation handler (skip AnnounceControl for class icons; let
//     the per-frame monitor speak when the cache is populated) to
//     also kill the stale-read echo.
//
//     Detection is positional: CSWGuiClassSelection isn't a CGuiInGame
//     slot (it's a top-level chargen panel), so IdentifyPanel never
//     resolves it. We check the panel's vtable directly and confirm
//     the focused control pointer lands on a multiple-of-0x25c offset
//     inside the class_selections[] array.
const char* TryClassSelectionIcon(void* control, void* owner,
                                  char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (IsClassSelectionIcon(owner, control)) {
        const char* cached = ClassLabelCacheLookup(owner, control);
        if (cached && cached[0] != '\0') {
            if (EmitText(outBuf, bufSize, cached)) {
                source = "perkind-classsel";
            }
        } else {
            // Cache miss. Populate only when active_control == icon AND
            // class_label is non-empty. CSWGuiClassSelection runs
            // OnEnterButton synchronously inside its SetActiveControl
            // override, so by the time active_control == icon is
            // observable, class_label has already been updated to the
            // matching class — this gives us the icon's CORRECT class
            // text in a single read. First-write-wins locking on the
            // cache then keeps subsequent reads stable, immune to the
            // engine's later bounces / class_label reverts.
            //
            // Guarded: `owner` passed IsPanelInManager, which proves it is
            // listed in panels[], not that the object is still alive.
            void* activeCtrl = nullptr;
            __try {
                activeCtrl = *reinterpret_cast<void**>(
                    reinterpret_cast<unsigned char*>(owner) +
                    kPanelActiveControlOffset);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                activeCtrl = nullptr;
            }
            if (activeCtrl == control) {
                void* classLabel =
                    reinterpret_cast<unsigned char*>(owner) +
                    kClassSelectionClassLabelOffset;
                char name[256];
                if (ExtractTextOrStrRefIndirect(classLabel,
                                                kLabelTextOffset,
                                                kLabelStrRefOffset,
                                                kLabelTextObjectOffset,
                                                name, sizeof(name)) &&
                    name[0] != '\0') {
                    // Append the class blurb the engine writes into LBL_DESC
                    // in the same OnEnterButton pass that set class_label —
                    // "Jedi Guardian" alone says nothing about what the class
                    // plays like, and the description is on screen for
                    // sighted players. It has to be captured HERE rather
                    // than read at announce time for exactly the reason
                    // class_label is: by the time the chain step announces an
                    // icon, both labels already hold the NEXT icon's text.
                    char text[kAnnounceTextMax];
                    acc::menus::detail::ComposeClassAnnounce(
                        owner, name, text, sizeof(text));
                    if (EmitText(outBuf, bufSize, text)) {
                        ClassLabelCacheStore(owner, control, text);
                        source = "perkind-classsel";
                        acclog::Write("Menus.PerKind",
                                      "ClassSelection cache+speak control=%p "
                                      "-> \"%s\"", control, text);
                    }
                }
            }
        }
    }
    return source;
}

// 9d. Per-kind value-display for the chargen portrait-selection panel
//     (CSWGuiPortraitCharGen, vtable=0x00759ea8). The two arrow buttons
//     (image-only, empty inline text) cycle the portrait but have no
//     own readable label — the visible "value" between them is a 3D
//     head scene. We anchor the chain on the LEFT arrow only (the
//     RIGHT arrow gets filtered out in RebindChain) and announce just
//     the current portrait value off the panel here, so the cycle UX
//     mirrors a normal `[◀] value [▶]` widget: one chain entry, Left
//     and Right cycle, focus on it reads the value.
//
//     The RIGHT arrow is intentionally returned without text — that's
//     what FindAdjacentArrow keys on (it skips controls that have
//     announceable text), so Left/Right routing in menus.cpp can find
//     the right_arrow as the cycle neighbour.
//
//     Live cycle state lives on the chargen creature: UpdatePortrait-
//     Button (0x006f8ad0) writes the new resref into CSWCObject.portrait
//     (+0xa8, inline CSWPortrait = CResRef = char[16]) on every cycle.
//     Reading those 16 bytes and parsing the `po_p[mf]h[abc]\d` pattern
//     gives us a description (e.g. "female asian 3").
//     The per-frame focused-control monitor re-extracts every tick —
//     when the resref changes, the composed string changes and the
//     diff fires speech.
//
//     Fallback ladder for the value: portrait_label.gui_string
//     (defensive — engine doesn't populate it today) → resref-pattern
//     parse → raw resref (non-PC / modded portraits) → numeric
//     portrait_id+1 (last resort, stable but uninformative).
const char* TryPortraitCharGenArrow(void* control, void* owner,
                                    char* outBuf, size_t bufSize) {
    const char* source = nullptr;
    if (owner) {
        if (acc::engine::HasVtable(owner, kVtableCSWGuiPortraitCharGen)) {
            auto* panelBase = reinterpret_cast<unsigned char*>(owner);
            auto* ctrlBase  = reinterpret_cast<unsigned char*>(control);
            ptrdiff_t off   = ctrlBase - panelBase;

            // Only the left_arrow is the value-display anchor. The
            // right_arrow returns nullptr from FromControl so it (a) gets
            // skipped by the chain rebind's text-presence check downstream,
            // and (b) is discoverable by FindAdjacentArrow as a text-less
            // cycle neighbour.
            if (off == (ptrdiff_t)kPortraitLeftArrowOffset) {
                // Try the panel's portrait_label first — engine doesn't
                // populate it today but we don't lose anything by checking.
                void* portraitLabel = panelBase + kPortraitLabelOffset;
                char portraitText[256];
                bool gotLabel = ExtractTextOrStrRefIndirect(
                    portraitLabel,
                    kLabelTextOffset,
                    kLabelStrRefOffset,
                    kLabelTextObjectOffset,
                    portraitText, sizeof(portraitText));

                // Get the live portrait state from panel.creature via the
                // engine accessors. Direct field reads on the creature
                // (+0xa8 CSWCObject.portrait, +0x14/+0x24 CSWCPlayer-style)
                // and on the panel (+0x1238 portrait_id) all stay zero
                // through cycling — only the engine accessors return live
                // values (verified in the patch log).
                //
                // GetPortrait fills a CResRef (16-byte char[]) with the
                // baseresref string for the currently-selected portrait,
                // regardless of the underlying storage location. This is
                // our preferred source — works for any portrait id the
                // engine cycles through, including rows our static
                // kPortraitByRow table is unsure about (e.g. id=32 turned
                // out to be a male PC portrait, not po_pbastila as the
                // data-section ordering had suggested).
                //
                // GetPortraitId is logged alongside as diagnostic context;
                // kPortraitByRow remains a defensive fallback for the
                // (theoretical) case where GetPortrait fails.
                uint32_t portraitId = 0xFFFFFFFFu;
                char liveResref[kResRefSize + 1] = {0};
                __try {
                    void* creature = *reinterpret_cast<void**>(
                        panelBase + kPortraitCharGenCreatureOffset);
                    if (creature) {
                        auto getPid = reinterpret_cast<
                            PFN_CSWCCreatureGetPortraitId>(
                                kAddrCSWCCreatureGetPortraitId);
                        portraitId = getPid(creature) & 0xFFFFu;

                        auto getP = reinterpret_cast<
                            PFN_CSWCCreatureGetPortrait>(
                                kAddrCSWCCreatureGetPortrait);
                        char tmp[kResRefSize] = {0};
                        getP(creature, tmp, /*side=*/0);
                        memcpy(liveResref, tmp, kResRefSize);
                        liveResref[kResRefSize] = '\0';
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    portraitId  = 0xFFFFFFFFu;
                    liveResref[0] = '\0';
                }

                // Pick the source: live resref from engine if non-empty,
                // else the static row → baseresref table for the rows we
                // know are correct.
                const char* mappedResref =
                    (liveResref[0] != '\0') ? liveResref : nullptr;

                // Parse the chargen stem: "po_p[mf]h[abch]\d+".
                // Index map:
                //   [0..3] = "po_p"
                //   [4]    = gender ('m'|'f')
                //   [5]    = 'h'
                //   [6]    = head family ('a'|'b'|'c', plus 'h' on K2)
                //   [7..]  = variant digits (ignored — see below)
                //
                // Three per-game shape differences, all found on the KOTOR 2
                // chargen rounds: K2 numbers the variants with a ZERO-PADDED
                // TWO-DIGIT field ("po_pmhc06" vs K1's "po_pmhc3"), spells
                // several stems in upper case ("po_PMHC06"), and adds a
                // FOURTH head family 'h' (P_MAL_H / P_FEM_H, four rows).
                // Reading a single char at [7] turned every K2 portrait into
                // variant "0", an exact-case compare would have dropped the
                // upper-case rows, and the unknown 'h' fell through to the
                // raw-resref branch — the user heard "Portrait: po_pmhh02".
                //
                // The variant number comes from PortraitVariantRank, not
                // from the stem: see the PortraitRow tables for why the
                // stem's own digits are not a position. The digits are the
                // fallback for rows no table knows (portrait mods).
                char description[128] = {0};
                bool parsedPattern = false;
                if (mappedResref && strnlen(mappedResref, 16) >= 8) {
                    char gender = AsciiLower(mappedResref[4]);
                    char family = AsciiLower(mappedResref[6]);
                    if (AsciiLower(mappedResref[0]) == 'p' &&
                        AsciiLower(mappedResref[1]) == 'o' &&
                        mappedResref[2] == '_' &&
                        AsciiLower(mappedResref[3]) == 'p' &&
                        (gender == 'm' || gender == 'f') &&
                        AsciiLower(mappedResref[5]) == 'h' &&
                        (family == 'a' || family == 'b' ||
                         family == 'c' || family == 'h') &&
                        mappedResref[7] >= '0' && mappedResref[7] <= '9') {
                        int variant = PortraitVariantRank(portraitId);
                        if (variant == 0) {
                            // Unknown row: fall back to the stem's own
                            // digits. Leading zeros drop out for free
                            // ("06" → 6), and the resref is a NUL-terminated
                            // CResRef copy so the loop always stops.
                            for (int i = 7;
                                 mappedResref[i] >= '0' && mappedResref[i] <= '9';
                                 ++i) {
                                variant = variant * 10 + (mappedResref[i] - '0');
                            }
                        }
                        auto genderId = (gender == 'f')
                            ? acc::strings::Id::PortraitGenderFemale
                            : acc::strings::Id::PortraitGenderMale;
                        acc::strings::Id familyId = acc::strings::Id::Count_;
                        switch (family) {
                            case 'a': familyId = acc::strings::Id::PortraitRaceAsian; break;
                            case 'b': familyId = acc::strings::Id::PortraitRaceDark;  break;
                            case 'c': familyId = acc::strings::Id::PortraitRaceLight; break;
                            case 'h': familyId = acc::strings::Id::PortraitRaceTypeH; break;
                        }
                        if (familyId != acc::strings::Id::Count_) {
                            snprintf(description, sizeof(description),
                                     acc::strings::Get(
                                         acc::strings::Id::FmtPortraitDescription),
                                     acc::strings::Get(genderId),
                                     acc::strings::Get(familyId),
                                     variant);
                            parsedPattern = true;
                        }
                    }
                }

                const char* label =
                    acc::strings::Get(acc::strings::Id::PortraitLabel);
                const char* fmtArrow =
                    acc::strings::Get(acc::strings::Id::FmtPortraitArrow);
                if (gotLabel && portraitText[0] != '\0') {
                    snprintf(outBuf, bufSize, fmtArrow,
                             label, portraitText);
                } else if (parsedPattern) {
                    snprintf(outBuf, bufSize, fmtArrow,
                             label, description);
                } else if (mappedResref) {
                    snprintf(outBuf, bufSize, fmtArrow,
                             label, mappedResref);
                } else if (portraitId != 0xFFFFFFFFu) {
                    snprintf(outBuf, bufSize,
                             acc::strings::Get(
                                 acc::strings::Id::FmtPortraitArrowId),
                             label, (int)(portraitId + 1));
                } else {
                    // Couldn't even read GetPortraitId — fall back to
                    // the panel's stale portrait_id field as a last
                    // resort so the chain entry isn't text-less.
                    uint32_t fallbackId =
                        ReadU32(owner, kPortraitIdOffset);
                    snprintf(outBuf, bufSize,
                             acc::strings::Get(
                                 acc::strings::Id::FmtPortraitArrowId),
                             label, (int)(fallbackId + 1));
                }
                if (outBuf[0] != '\0') {
                    source = "perkind-portrait";
                    acclog::Write("Menus.PerKind",
                                  "PortraitCharGen control=%p anchor=left "
                                  "id=%u resref=%s -> \"%s\"",
                                  control, portraitId,
                                  mappedResref ? mappedResref : "<null>",
                                  outBuf);
                }
            }
        }
    }
    return source;
}

}  // namespace acc::menus::extract
