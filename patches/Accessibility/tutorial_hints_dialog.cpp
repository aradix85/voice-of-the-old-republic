// The dialogue-line side of the tutorial hints: the mouse-worded tutorial
// lines replaced by a keyboard hint popup, the voiceless subtitle lines
// forced spoken (K1), and the voiced lines whose subtitle is suppressed (K2).

#include "tutorial_hints.h"

#include <cstdint>
#include <cstring>

#include "engine_game.h"     // IsKotor2
#include "engine_reads.h"    // LookupTlk, EqualsTrimmed
#include "log.h"
#include "strings.h"

namespace acc::tutorial_hints {

namespace {

using acc::strings::Id;

// Surface 2 map: source strref -> hint Id. Resolved to the current language's
// text lazily via the engine TLK, then matched against the rendered dialogue
// line. KOTOR 1: the Endar Spire Trask / pop-window mouse lines. KOTOR 2: the
// Ebon Hawk prologue's T3-M4 instruction lines (module 001EBO) — the K2
// equivalent of the Trask set.
struct DialogHint {
    uint32_t strref;
    Id       id;
};
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

// KOTOR 2 — every strref is a T3-M4 instruction line from the Ebon Hawk
// prologue (001EBO), mined from the module's _dlg.erf 2026-08-25. Most lines
// exist twice, once worded for the PC (mouse + vanilla default keys) and once
// for the Xbox pad; both variants map to the same hint id, so whichever the
// engine renders is caught. The spoken vanilla line is suppressed for these
// (see dialog_speech.cpp — T3 is a droid, so the human-VO suppression that
// covers Trask on KOTOR 1 does not apply) and the hint fires as a popup at
// the reply break, exactly like the Trask set.
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
    kDialogHintCountK1 > kDialogHintCountK2 ? kDialogHintCountK1
                                            : kDialogHintCountK2;

// The running game's table. Selected once — the game cannot change mid-process.
inline const DialogHint* ActiveDialogHints(int* count) {
    if (acc::game::IsKotor2()) {
        *count = kDialogHintCountK2;
        return kDialogHintsK2;
    }
    *count = kDialogHintCountK1;
    return kDialogHintsK1;
}

// Resolved-text cache, built once the TLK is loaded.
struct ResolvedHint {
    char text[512];
    Id   id;
    bool valid;
};
ResolvedHint s_resolved[kDialogHintMax];
int          s_resolvedCount = 0;
bool         s_resolvedBuilt = false;

// Guards against a stray padding space between the engine's rendered label
// and its own TLK resolution of the same strref. Shared implementation —
// see engine_reads.h.
using acc::engine::EqualsTrimmed;

void BuildResolvedIfNeeded() {
    if (s_resolvedBuilt) return;
    int count = 0;
    const DialogHint* hints = ActiveDialogHints(&count);
    int resolvedCount = 0;
    for (int i = 0; i < count; ++i) {
        s_resolved[i].id    = hints[i].id;
        s_resolved[i].valid = false;
        s_resolved[i].text[0] = '\0';
        if (acc::engine::LookupTlk(hints[i].strref, s_resolved[i].text,
                                   sizeof(s_resolved[i].text))) {
            s_resolved[i].valid = true;
            ++resolvedCount;
        }
    }
    // Only latch "built" once we actually resolved at least one entry — before
    // the TLK table is live (main menu / early load) every lookup fails and we
    // must retry on a later call.
    if (resolvedCount > 0) {
        s_resolvedBuilt = true;
        s_resolvedCount = count;
        acclog::Write("TutorialHint", "dialog hint map built: %d/%d strrefs resolved",
                      resolvedCount, count);
    }
}

}  // namespace

// ---- Forced-spoken VO-less subtitle lines --------------------------------
//
// Curated strrefs whose dialogue node ships as subtitle text with no VO. The
// speaker (Trask) classifies as a voiced human, so dialog_speech's human-
// subtitle suppression would silence these entirely. We resolve each strref to
// the current locale's text once (same lazy TLK path as the hint map) and
// trim-match it against the rendered line so dialog_speech can force it spoken.
//
//   39454 — "You have enough experience to level up. Level up
//            before you go through this door." Trask's Endar Spire level-up-gate
//            line, said when the player tries the locked door before assigning
//            their first level. No VO recorded; subtitle only. (verified in the
//            German dialog.tlk, 2026-07-19)
constexpr uint32_t kForcedSpokenStrrefs[] = { 39454 };
constexpr int kForcedSpokenCount =
    static_cast<int>(sizeof(kForcedSpokenStrrefs) / sizeof(kForcedSpokenStrrefs[0]));

char s_forcedText[kForcedSpokenCount][512];
bool s_forcedTextBuilt = false;

void BuildForcedTextIfNeeded() {
    if (s_forcedTextBuilt) return;
    int resolved = 0;
    for (int i = 0; i < kForcedSpokenCount; ++i) {
        s_forcedText[i][0] = '\0';
        if (acc::engine::LookupTlk(kForcedSpokenStrrefs[i], s_forcedText[i],
                                   sizeof(s_forcedText[i]))) {
            ++resolved;
        }
    }
    if (resolved > 0) {
        s_forcedTextBuilt = true;
        acclog::Write("TutorialHint", "forced-spoken map built: %d/%d strrefs resolved",
                      resolved, kForcedSpokenCount);
    }
}

const char* HintForDialogLine(const char* renderedLine, uint32_t* outStrref) {
    if (!renderedLine || !renderedLine[0]) return nullptr;
    BuildResolvedIfNeeded();
    if (!s_resolvedBuilt) return nullptr;
    int count = 0;
    const DialogHint* hints = ActiveDialogHints(&count);
    for (int i = 0; i < s_resolvedCount; ++i) {
        if (!s_resolved[i].valid) continue;
        if (EqualsTrimmed(renderedLine, s_resolved[i].text)) {
            const char* s = acc::strings::Get(s_resolved[i].id);
            if (!s[0]) return nullptr;
            // s_resolved is built parallel to the active hint table, so index
            // i carries the source strref.
            if (outStrref) *outStrref = hints[i].strref;
            return s;
        }
    }
    return nullptr;
}

bool IsForcedSpokenDialogLine(const char* renderedLine) {
    if (!renderedLine || !renderedLine[0]) return false;
    // The curated strrefs are KOTOR 1 lines; on KOTOR 2 the same numbers name
    // unrelated strings, so resolving them would arm the matcher with garbage.
    if (acc::game::IsKotor2()) return false;
    BuildForcedTextIfNeeded();
    if (!s_forcedTextBuilt) return false;
    for (int i = 0; i < kForcedSpokenCount; ++i) {
        if (s_forcedText[i][0] && EqualsTrimmed(renderedLine, s_forcedText[i])) {
            return true;
        }
    }
    return false;
}

// ---- Voiced subtitle lines to suppress ------------------------------------
//
// The INVERSE of the forced-spoken list: curated strrefs whose line carries
// real recorded voice-over, but whose speaker the classifier cannot resolve —
// so the human-VO suppression never fires and the mod speaks the subtitle
// over the actor. First entries (2026-08-26): the KOTOR 2 Ebon Hawk
// prologue's narrator, "the voice of the Ebon Hawk" (strref 134048 names her)
// — her lines render as ownerless barks and cinematic-copy panels
// (speaker=[?], appearance -1), and every one below has its VO file verified
// present under StreamVoice/001/. The prologue's silent lines (the security
// consoles, whose VO resrefs have no audio on disk) are deliberately NOT
// here: with no voice behind them, the subtitle is the only channel.
//
// Tutorial-mapped lines (kDialogHintsK2) are also not here — dialog_speech
// suppresses those through the hint match and replaces them with a popup.
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
constexpr int kVoicedSuppressedCount = static_cast<int>(
    sizeof(kVoicedSuppressedK2) / sizeof(kVoicedSuppressedK2[0]));

char s_voicedText[kVoicedSuppressedCount][512];
bool s_voicedTextBuilt = false;

void BuildVoicedTextIfNeeded() {
    if (s_voicedTextBuilt) return;
    int resolved = 0;
    for (int i = 0; i < kVoicedSuppressedCount; ++i) {
        s_voicedText[i][0] = '\0';
        if (acc::engine::LookupTlk(kVoicedSuppressedK2[i], s_voicedText[i],
                                   sizeof(s_voicedText[i]))) {
            ++resolved;
        }
    }
    if (resolved > 0) {
        s_voicedTextBuilt = true;
        acclog::Write("TutorialHint", "voiced-suppress map built: %d/%d strrefs resolved",
                      resolved, kVoicedSuppressedCount);
    }
}

// memcmp-based strstr for a length-bounded needle segment.
const char* FindSegment(const char* hay, const char* needle, size_t n) {
    if (n == 0) return hay;
    size_t hayLen = std::strlen(hay);
    if (hayLen < n) return nullptr;
    for (size_t i = 0; i + n <= hayLen; ++i) {
        if (std::memcmp(hay + i, needle, n) == 0) return hay + i;
    }
    return nullptr;
}

// EqualsTrimmed, extended for lines the engine token-substitutes at render
// time (<FullName> and friends, e.g. the treatinj medbay lines): the stored
// TLK text's literal segments must appear left to right in the rendered line,
// a token matches any run of characters, and — like EqualsTrimmed — leading /
// trailing padding spaces don't count. Degenerates to EqualsTrimmed when the
// stored text has no token.
bool MatchesWithTokens(const char* rendered, const char* stored) {
    if (std::strchr(stored, '<') == nullptr) {
        return EqualsTrimmed(rendered, stored);
    }
    while (*rendered == ' ') ++rendered;
    while (*stored == ' ') ++stored;
    const char* r = rendered;
    const char* s = stored;
    bool first = true;
    bool endsWithToken = false;
    while (*s) {
        if (*s == '<') {
            const char* close = std::strchr(s, '>');
            if (!close) { endsWithToken = true; break; }  // malformed tail
            s = close + 1;
            endsWithToken = (*s == '\0');
            first = false;
            continue;
        }
        const char* tok = std::strchr(s, '<');
        size_t segLen = tok ? static_cast<size_t>(tok - s) : std::strlen(s);
        size_t cmpLen = segLen;
        if (!tok) {  // final literal: ignore its trailing padding
            while (cmpLen > 0 && s[cmpLen - 1] == ' ') --cmpLen;
        }
        if (first) {
            if (std::strncmp(r, s, cmpLen) != 0) return false;
            r += cmpLen;
        } else {
            const char* hit = FindSegment(r, s, cmpLen);
            if (!hit) return false;
            r = hit + cmpLen;
        }
        s += segLen;
        first = false;
    }
    if (endsWithToken) return true;  // a trailing token swallows the rest
    while (*r == ' ') ++r;           // after the final literal: padding only
    return *r == '\0';
}

bool IsVoicedSuppressedLine(const char* renderedLine) {
    if (!renderedLine || !renderedLine[0]) return false;
    // The curated strrefs are KOTOR 2 lines; a future KOTOR 1 entry gets its
    // own table (see the forced-spoken list for the mirror-image gate).
    if (!acc::game::IsKotor2()) return false;
    BuildVoicedTextIfNeeded();
    if (!s_voicedTextBuilt) return false;
    for (int i = 0; i < kVoicedSuppressedCount; ++i) {
        if (s_voicedText[i][0] &&
            MatchesWithTokens(renderedLine, s_voicedText[i])) {
            return true;
        }
    }
    return false;
}

}  // namespace acc::tutorial_hints
