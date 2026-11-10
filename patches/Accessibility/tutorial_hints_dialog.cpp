// The dialogue-line side of the tutorial hints: the mouse-worded tutorial
// lines replaced by a keyboard hint popup, the voiceless subtitle lines
// forced spoken (K1), and the voiced lines whose subtitle is suppressed (K2).
// Every list is keyed by source strref and matched in the game's own wording
// through the TLK.

#include "tutorial_hints_internal.h"

#include <cstdint>
#include <cstring>

#include "engine_game.h"  // IsKotor2
#include "strings.h"
#include "text_match.h"

namespace acc::tutorial_hints {

namespace {

using acc::strings::Id;
using acc::text::EqualsTrimmed;
using acc::text::MatchesWithTokens;

struct DialogHint {
    uint32_t strref;
    Id       id;
};

// KOTOR 1: the Endar Spire Trask lines and Carth's stealth line on Taris.
constexpr DialogHint kDialogHintsK1[] = {
    // Closing line of Trask's intro tree ("grab your things, we have to go").
    // Not a rewritten tutorial line — this is stock story VO — but it's the
    // point where the intro conversation ends and the player is first handed
    // control, so we hang the core-controls popup off it (fires on the
    // dialogue-close flush, since the tree ends here with no reply break).
    {10326, Id::TutTraskGettingStarted},
    {48330, Id::TutTraskEquipOpen},
    {48331, Id::TutTraskEquipBrowse},
    {48332, Id::TutTraskEquipWeapon},
    {48344, Id::TutTraskCamera},
    {48345, Id::TutTraskFootlocker},
    {48348, Id::TutTraskPickItem},
    {48350, Id::TutTraskLeader},
    {48351, Id::TutTraskMakeLeader},
    {48353, Id::TutTraskMenus},
    {48354, Id::TutTraskTabs},
    {48360, Id::TutTraskActionMenu},
    {48363, Id::TutTraskMedkit},
    {48441, Id::TutTraskOpenDoor},
    {48544, Id::TutTraskActivateEntry},
    {48547, Id::TutTraskTargetMenu},
    {48550, Id::TutTraskConfirmEntry},
    {48551, Id::TutTraskPaused},
    {48552, Id::TutTraskWalkTo},
    {48555, Id::TutTraskSecurity},
    {48556, Id::TutTraskHealWounded},
    {48324, Id::TutLevelUp},
    // Taris — Carth's stealth tutorial. His mouse-worded "click the
    // Stealth ability to activate it" line becomes the G-key hint,
    // fired as a popup at the reply break / dialogue close like the Trask set.
    {48557, Id::TutStealthMode},
};

// KOTOR 2: the Ebon Hawk prologue's T3-M4 instruction lines (001EBO). Most
// exist twice, worded for the PC and for the Xbox pad, and both map to the
// same hint. T3 is a droid, so the human-VO suppression that covers Trask
// does not apply: dialog_speech suppresses these through the hint match and
// the hint fires as a popup at the reply break, like the Trask set.
constexpr DialogHint kDialogHintsK2[] = {
    {130726, Id::TutHintMovement},       // controls.dlg (PC)
    {130727, Id::TutHintMovement},       // controls.dlg (Xbox)
    {130735, Id::TutTraskFootlocker},    // target.dlg (PC) — use objects
    {130736, Id::TutTraskFootlocker},    // target.dlg (Xbox)
    {124183, Id::TutK2DialogueChoice},   // 3cfd_int.dlg (PC)
    {124184, Id::TutK2DialogueChoice},   // 3cfd_int.dlg (Xbox)
    {124181, Id::TutTraskLeader},        // 3cfd_end.dlg — party switching
    {124781, Id::TutHintAttack},         // combat.dlg (PC)
    {124782, Id::TutHintAttack},         // combat.dlg (Xbox)
    {132460, Id::TutHintAttack},         // combat.dlg (Xbox variant 2)
    {124786, Id::TutK2Pause},            // combat.dlg — pausing
    {124787, Id::TutK2Pause},            // combat.dlg (Xbox: WHITE button)
    {124788, Id::TutHintGrenade},        // combat.dlg (Xbox)
    {124789, Id::TutHintGrenade},        // combat.dlg (PC)
    {124783, Id::TutK2WeaponConfig},     // combat.dlg (PC) — two weapon configs
    {124784, Id::TutK2WeaponConfig},     // combat.dlg (Xbox)
    {124793, Id::TutK2EquipWeapon},      // combat.dlg — pick a weapon
    {124794, Id::TutK2EquipWeapon},      // combat.dlg (Xbox: START menu)
    {130047, Id::TutK2EquipWeapon},      // sens_drd.dlg — T3 needs a blaster
    {130048, Id::TutK2EquipWeapon},      // sens_drd.dlg (Xbox)
    {126827, Id::TutK2EquipWeapon},      // flameth.dlg — droid flamethrower
    {126828, Id::TutK2EquipWeapon},      // flameth.dlg (Xbox)
    { 74999, Id::TutTraskEquipOpen},     // minecntr.dlg — found armor
    {126834, Id::TutTraskEquipOpen},     // minecntr.dlg (Xbox)
    {123746, Id::TutK2TakeItem},         // plscylsp.dlg (PC) — the probe
    {123747, Id::TutK2TakeItem},         // plscylsp.dlg (Xbox)
    {123744, Id::TutK2Journal},          // journal.dlg — new quests (J on K2)
    {123743, Id::TutK2Journal},          // journal.dlg (Xbox)
    {130036, Id::TutHintMapScreen},      // map.dlg — map help
    {130037, Id::TutHintMapScreen},      // map.dlg (Xbox)
    {130030, Id::TutK2GalaxyMap},        // glxy_map.dlg (PC)
    {132456, Id::TutK2GalaxyMap},        // glxy_map.dlg (Xbox)
    {124804, Id::TutK2SoloMode},         // gar_door.dlg — solo-mode door
    {124805, Id::TutK2SoloMode},         // gar_door.dlg (Xbox: BACK button)
    {128196, Id::TutK2RepairKit},        // rep_kit.dlg (PC)
    {128197, Id::TutK2RepairKit},        // rep_kit.dlg (Xbox)
    {127843, Id::TutK2SecuritySpike},    // sectun.dlg (PC)
    {126838, Id::TutK2SecuritySpike},    // sectun.dlg (Xbox)
    {126837, Id::TutK2SecuritySpike},    // sectun.dlg — "lock won't open" variant
    {128553, Id::TutHintBash},           // fl_bash.dlg (PC)
    {123737, Id::TutHintBash},           // fl_bash.dlg (Xbox)
    {126337, Id::TutHintBash},           // fl_bash.dlg — "find a weapon" variant
    { 74965, Id::TutHintMine},           // blast.dlg — blow the door with a mine
    // Deliberately unmapped: 124798 (the damaged comm-console header) — its
    // one mouse clause is minor and the flavor text should stay audible.
};

constexpr int kDialogHintCountK1 =
    static_cast<int>(sizeof(kDialogHintsK1) / sizeof(kDialogHintsK1[0]));
constexpr int kDialogHintCountK2 =
    static_cast<int>(sizeof(kDialogHintsK2) / sizeof(kDialogHintsK2[0]));
constexpr int kDialogHintMax =
    kDialogHintCountK1 > kDialogHintCountK2 ? kDialogHintCountK1 : kDialogHintCountK2;

const DialogHint* ActiveDialogHints(int* count) {
    if (acc::game::IsKotor2()) {
        *count = kDialogHintCountK2;
        return kDialogHintsK2;
    }
    *count = kDialogHintCountK1;
    return kDialogHintsK1;
}

TlkTexts<kDialogHintMax, 512> s_hintText;

// KOTOR 1 subtitle lines shipped without VO whose speaker (Trask) counts as
// a voiced human, so the human-subtitle suppression would silence them.
//   39454: Trask's Endar Spire level-up gate, said when the player tries the
//          locked door before taking the first level.
constexpr uint32_t kForcedSpokenStrrefs[] = {39454};
constexpr int kForcedSpokenCount =
    static_cast<int>(sizeof(kForcedSpokenStrrefs) / sizeof(kForcedSpokenStrrefs[0]));
TlkTexts<kForcedSpokenCount, 512> s_forcedText;

// The inverse: KOTOR 2 lines with real recorded VO whose speaker the
// classifier cannot resolve, so the mod would speak the subtitle over the
// actor. The Ebon Hawk prologue's narrator, "the voice of the Ebon Hawk"
// (134048 names her): ownerless barks and cinematic-copy panels (speaker
// [?], appearance -1), each with its VO file under StreamVoice/001/. The
// silent security consoles (no audio on disk) stay out, as do the
// tutorial-mapped lines above.
constexpr uint32_t kVoicedSuppressedK2[] = {
    74957, 74958, 124138, 124139, 125865, 125866, 125867, 125868,  // 3cfd
    125872, 125874, 125889, 127840, 127841,
    124182,  // 3cfd_end
    124185, 125637,  // 3cfd_int
    124785, 124791, 124792, 124796, 124797, 126017, 131370, 131371,  // combat
    131372,
    131373,  // comcon
    126331, 126332, 126333, 126334, 126335, 126336,  // comp_voc (ship status)
    118948, 118949, 132430,  // eng_door
    117948, 130728,  // eng_port
    117949,  // eng_star
    128552,  // exit
    132431, 132432, 132433, 132434, 132435, 132436, 132437, 132438,  // extra
    132439, 132440, 132441, 132442, 132443, 132444, 132445, 132446,  // (nav +
    132447, 132448, 132449, 132450, 132451, 132452, 132453, 132454,  //  areas)
    132455,
    123738,  // fl_bash (workbench aside)
    124806,  // gar_door (hull-damage flavor)
    123740,  // glxy_map (map intro)
    124187,  // got_meds
    74991,  // hk50dr
    74992, 117953, 117954, 117955, 117960, 117961, 117962, 123361,  // hyper
    130033, 131379,
    74993,  // impdoor
    118950, 118951, 118952, 125643, 125645, 125647, 130034, 130035,  // intro
    131380, 134048,
    116311,  // kre_body
    126831,  // lift_002
    124192,  // minecntr (needs-key flavor)
    125708,  // no_mines
    130040,  // outro
    75004,  // pcbody
    75005,  // pilotdr
    75006,  // plscylsp (container aside)
    75010,  // secdoor
    75011,  // secdoor2
    75012,  // secspk
    132458,  // sectun (difficulty aside)
    130049,  // sens_drd ("now hostile")
    128556, 128557, 128558, 128559, 128560, 128561, 128562, 128563,  // sparkwir
    131383,  // stardoor
    123776,  // tr_lift
    117970, 117971, 117972, 117973, 117974, 117975,  // treatinj (<FullName>)
    123780, 123781, 123782,  // wrkbnch
};
constexpr int kVoicedSuppressedCount =
    static_cast<int>(sizeof(kVoicedSuppressedK2) / sizeof(kVoicedSuppressedK2[0]));
TlkTexts<kVoicedSuppressedCount, 512> s_voicedText;

}  // namespace

const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref) {
    if (!renderedLine || !renderedLine[0]) return nullptr;
    int count = 0;
    const DialogHint* hints = ActiveDialogHints(&count);
    if (!s_hintText.Ensure(count, [&](int i) { return hints[i].strref; }, "dialog hint")) {
        return nullptr;
    }
    const int i = s_hintText.Find(
        [&](const char* stored) { return EqualsTrimmed(renderedLine, stored); });
    if (i < 0) return nullptr;
    const char* s = acc::strings::Get(hints[i].id);
    if (!s[0]) return nullptr;
    if (outStrref) *outStrref = hints[i].strref;
    return s;
}

bool IsForcedSpokenDialogLine(const char* renderedLine) {
    // KOTOR 1 strrefs: on KOTOR 2 the same numbers name unrelated strings.
    if (!renderedLine || !renderedLine[0] || acc::game::IsKotor2()) return false;
    return s_forcedText.Ensure(kForcedSpokenCount,
                               [](int i) { return kForcedSpokenStrrefs[i]; }, "forced-spoken") &&
           s_forcedText.Find([&](const char* stored) {
               return EqualsTrimmed(renderedLine, stored);
           }) >= 0;
}

bool IsVoicedSuppressedLine(const char* renderedLine) {
    if (!renderedLine || !renderedLine[0] || !acc::game::IsKotor2()) return false;
    return s_voicedText.Ensure(kVoicedSuppressedCount,
                               [](int i) { return kVoicedSuppressedK2[i]; }, "voiced-suppress") &&
           s_voicedText.Find([&](const char* stored) {
               return MatchesWithTokens(renderedLine, stored);
           }) >= 0;
}

}  // namespace acc::tutorial_hints
