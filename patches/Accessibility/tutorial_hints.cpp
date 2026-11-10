#include "tutorial_hints.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

#include "engine_game.h"     // IsKotor2 — per-game strref/row tables
#include "engine_reads.h"    // LookupTlk
#include "log.h"
#include "strings.h"
#include "engine_offsets_select.h"

namespace acc::tutorial_hints {

namespace {

using acc::strings::Id;

// CSWGuiTutorialBox +0x994 (uint8) — the source tutorial.2da row index the
// popup was built from. RE-confirmed: SetTutorialReason writes it, SetNextMessage
// reads it back as the 2DA row. Survives after the popup is built, so a poll
// recovers the popup's identity language-independently.
// KOTOR 2: +0x9d8, witnessed in both directions — SetTutorialReason
// @0x00898f60 writes it (and zeroes the page counter at +0x9d9), and
// SetNextMessage @0x00899630 reads it back as the row for its
// "Message_PC%i" 2DA lookups.
const size_t kTutorialBoxRowOffset = acc::off::Pick(0x994, 0x9d8);

using acc::engine::EqualsTrimmed;

}  // namespace

int ReadTutorialRow(void* panel) {
    if (!panel) return -1;
    __try {
        return *reinterpret_cast<uint8_t*>(
            reinterpret_cast<unsigned char*>(panel) + kTutorialBoxRowOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

const char* HintForTutorialRow(int row) {
    // Rows 0-42 carry the same labels in both games' tutorial.2da (verified
    // against the K2 2DA 2026-08-25), so this block is shared. Rows only
    // KOTOR 2 fires — its extra rows 43+, plus rows that exist in KOTOR 1's
    // 2DA but were left unmapped there — follow in the K2-only block.
    Id id;
    bool have = true;
    switch (row) {
        case 0:  id = Id::TutHintCombatFeat;       break;  // Use_Combat_Feat
        case 1:  id = Id::TutHintGrenade;          break;  // Use_Grenade
        case 2:  id = Id::TutHintMine;             break;  // Set_Mine
        case 3:  id = Id::TutHintFriendlyPower;    break;  // Use_Friendly_Force_Power
        case 4:  id = Id::TutHintHostilePower;     break;  // Use_Hostile_Force_Power
        case 5:  id = Id::TutHintActionMenuScroll; break;  // Use_Vertical_Menu
        // Rows 7/12/14 name the Messages/Quests keys, which KOTOR 2's keymap
        // swaps (Quests J, Messages L — mirror of KOTOR 1), so they pick the
        // J/L-swapped K2 ids there.
        case 7:                                            // Press_Start
            id = acc::game::IsKotor2() ? Id::TutK2OpenScreens
                                       : Id::TutHintOpenScreens;
            break;
        case 8:  id = Id::TutHintCycleTargets;     break;  // Use_Triggers_To_Cycle...
        case 11: id = Id::TutHintEquipSlot;        break;  // Press_A_On_Equip_Screen
        case 20: id = Id::TutHintInventory;        break;  // party stash / inventory
        case 12:                                           // Enter_Messages_Screen
            id = acc::game::IsKotor2() ? Id::TutK2Messages
                                       : Id::TutHintMessages;
            break;
        case 13: id = Id::TutHintMapScreen;        break;  // Enter_Map_Screen
        case 14:                                           // Receive_Journal_Entry
            id = acc::game::IsKotor2() ? Id::TutK2Journal
                                       : Id::TutHintJournal;
            break;
        case 15: id = Id::TutHintPartyDies;        break;  // Party_Member_Dies
        case 21: id = Id::TutHintEnemyNear;        break;  // Hostile_Creature_Encountered
        case 33: id = Id::TutHintBash;             break;  // Bash
        case 34: id = Id::TutHintAttack;           break;  // Attack
        case 35: id = Id::TutHintAttackAuto;       break;  // Attack_Button_Mash
        case 42: id = Id::TutHintMovement;         break;  // Movement_Keys
        default: have = false;                     break;
    }
    if (!have && acc::game::IsKotor2()) {
        have = true;
        switch (row) {
            case 9:            id = Id::TutK2SwoopRace;    break;  // Start_Swoop_Race
            case 29: case 30:  id = Id::TutK2ActionQueue;  break;  // X-button queue
            case 31: case 32:  id = Id::TutK2ActionRemove; break;  // Y-button remove
            case 43:           id = Id::TutK2PlasmaTorch;  break;  // Bash_Plasma_Torch
            case 44:           id = Id::TutK2FirstPerson;  break;  // First_Person_Hint
            case 45:           id = Id::TutK2SpaceSuit;    break;  // K2_Temp3
            case 48: case 57:  id = Id::TutK2CommRoom;     break;  // K2_Temp6/15 (same strref)
            case 49:           id = Id::TutK2Mines;        break;  // K2_Temp7
            case 52:           id = Id::TutK2Vibrocutter;  break;  // Vibrocutter_Hint
            case 58:           id = Id::TutTraskMedkit;    break;  // Medpac_hint
            default:           have = false;               break;
        }
    }
    if (!have) return nullptr;
    const char* s = acc::strings::Get(id);
    return (s[0]) ? s : nullptr;
}

// The message strrefs (the Message columns of tutorial.2da) for every mapped
// row, paired with that row's keyboard-hint Id. The single-row message listbox
// announces these mouse-worded strings; we resolve them through the engine TLK
// so we can (a) suppress the raw announce on the popup's first-sight listbox
// row, and (b) substitute the keyboard hint when the user arrow-navigates onto
// the message in the popup's chain. Covers every page of the multi-message rows
// (K1: Attack 48566+42458, Attack_Button_Mash 48567+48568).
struct MouseMsg {
    uint32_t strref;
    Id       hint;
};
constexpr MouseMsg kTutorialMouseMsgsK1[] = {
    {48559, Id::TutHintCombatFeat},       // row 0  Use_Combat_Feat
    {48560, Id::TutHintGrenade},          // row 1  Use_Grenade
    {48366, Id::TutHintMine},             // row 2  Set_Mine
    {48367, Id::TutHintFriendlyPower},    // row 3  Use_Friendly_Force_Power
    {48563, Id::TutHintHostilePower},     // row 4  Use_Hostile_Force_Power
    {48564, Id::TutHintActionMenuScroll}, // row 5  Use_Vertical_Menu
    {48371, Id::TutHintOpenScreens},      // row 7  Press_Start
    {48372, Id::TutHintCycleTargets},     // row 8  Use_Triggers_To_Cycle...
    {48374, Id::TutHintEquipSlot},        // row 11 Press_A_On_Equip_Screen
    {42136, Id::TutHintInventory},        // row 20 party stash / inventory
    {41880, Id::TutHintMessages},         // row 12 Enter_Messages_Screen
    {41881, Id::TutHintMapScreen},        // row 13 Enter_Map_Screen
    {41882, Id::TutHintJournal},          // row 14 Receive_Journal_Entry
    {41883, Id::TutHintPartyDies},        // row 15 Party_Member_Dies
    {48375, Id::TutHintEnemyNear},        // row 21 Hostile_Creature_Encountered
    {48565, Id::TutHintBash},             // row 33 Bash
    {48566, Id::TutHintAttack},           // row 34 Attack page 0
    {42458, Id::TutHintAttack},           // row 34 Attack page 1
    {48567, Id::TutHintAttackAuto},       // row 35 Attack_Button_Mash page 0
    {48568, Id::TutHintAttackAuto},       // row 35 Attack_Button_Mash page 1
    {49121, Id::TutHintMovement},         // row 42 Movement_Keys
};

// KOTOR 2's tutorial.2da splits the columns into message_pc* and
// message_xbox* (mined 2026-08-25). The engine reads message_pc on PC
// (SetNextMessage @0x00899630 formats "Message_PC%i" unconditionally), but
// both variants are listed where they differ so a build that ever renders the
// pad column is still caught — text matching is cheap and cannot collide.
constexpr MouseMsg kTutorialMouseMsgsK2[] = {
    {49009, Id::TutHintCombatFeat},       // row 0  pc
    {41868, Id::TutHintCombatFeat},       // row 0  xbox
    {49010, Id::TutHintGrenade},          // row 1  pc
    {41869, Id::TutHintGrenade},          // row 1  xbox
    {49011, Id::TutHintMine},             // row 2  pc
    {41870, Id::TutHintMine},             // row 2  xbox
    {49012, Id::TutHintFriendlyPower},    // row 3  pc
    {41871, Id::TutHintFriendlyPower},    // row 3  xbox
    {49013, Id::TutHintHostilePower},     // row 4  pc
    {41872, Id::TutHintHostilePower},     // row 4  xbox
    {49014, Id::TutHintActionMenuScroll}, // row 5  pc
    {41873, Id::TutHintActionMenuScroll}, // row 5  xbox
    {49015, Id::TutK2OpenScreens},        // row 7  pc (J/L swapped)
    {41875, Id::TutK2OpenScreens},        // row 7  xbox
    {49016, Id::TutHintCycleTargets},     // row 8  pc
    {41876, Id::TutHintCycleTargets},     // row 8  xbox
    {49017, Id::TutK2SwoopRace},          // row 9  pc
    {41877, Id::TutK2SwoopRace},          // row 9  xbox
    {49018, Id::TutHintEquipSlot},        // row 11 pc
    {41879, Id::TutHintEquipSlot},        // row 11 xbox
    {41880, Id::TutK2Messages},           // row 12 (shared column; L on K2)
    {41881, Id::TutHintMapScreen},        // row 13 (shared column)
    {41882, Id::TutK2Journal},            // row 14 (shared column; J on K2)
    {41883, Id::TutHintPartyDies},        // row 15 (shared column)
    {42136, Id::TutHintInventory},        // row 20 (shared column)
    {49019, Id::TutHintEnemyNear},        // row 21 pc
    {42137, Id::TutHintEnemyNear},        // row 21 xbox
    {49021, Id::TutK2ActionQueue},        // row 29 pc
    {49022, Id::TutK2ActionQueue},        // row 30 pc
    {42461, Id::TutK2ActionQueue},        // rows 29/30 xbox
    {49023, Id::TutK2ActionRemove},       // rows 31/32 pc
    {42462, Id::TutK2ActionRemove},       // rows 31/32 xbox
    {49024, Id::TutHintBash},             // row 33 pc
    {42174, Id::TutHintBash},             // row 33 xbox
    {49025, Id::TutHintAttack},           // row 34 page 0 pc
    {42457, Id::TutHintAttack},           // row 34 page 0 xbox
    {42458, Id::TutHintAttack},           // row 34 page 1 (shared column)
    {49026, Id::TutHintAttackAuto},       // row 35 page 0 pc
    {42459, Id::TutHintAttackAuto},       // row 35 page 0 xbox
    {49027, Id::TutHintAttackAuto},       // row 35 page 1 pc
    {42460, Id::TutHintAttackAuto},       // row 35 page 1 xbox
    {49028, Id::TutHintMovement},         // row 42 pc (no xbox column)
    {48221, Id::TutK2PlasmaTorch},        // row 43 (shared column)
    {48222, Id::TutK2FirstPerson},        // row 44 (shared column)
    {48223, Id::TutK2SpaceSuit},          // row 45 (shared column)
    {48226, Id::TutK2CommRoom},           // rows 48+57 (shared column)
    {48227, Id::TutK2Mines},              // row 49 (shared column)
    {48230, Id::TutK2Vibrocutter},        // row 52 (shared column)
    {48629, Id::TutTraskMedkit},          // row 58 (shared column)
};

constexpr int kTutorialMouseMsgCountK1 =
    static_cast<int>(sizeof(kTutorialMouseMsgsK1) / sizeof(kTutorialMouseMsgsK1[0]));
constexpr int kTutorialMouseMsgCountK2 =
    static_cast<int>(sizeof(kTutorialMouseMsgsK2) / sizeof(kTutorialMouseMsgsK2[0]));
constexpr int kTutorialMouseMsgMax =
    kTutorialMouseMsgCountK1 > kTutorialMouseMsgCountK2 ? kTutorialMouseMsgCountK1
                                                        : kTutorialMouseMsgCountK2;

inline const MouseMsg* ActiveMouseMsgs(int* count) {
    if (acc::game::IsKotor2()) {
        *count = kTutorialMouseMsgCountK2;
        return kTutorialMouseMsgsK2;
    }
    *count = kTutorialMouseMsgCountK1;
    return kTutorialMouseMsgsK1;
}

// 1024, not 512: KOTOR 2's longest mapped popup text (the mine tutorial,
// 48227) is ~850 bytes, and the announce paths hand rendered text around in
// kAnnounceTextMax (1024) buffers — the stored copy must not truncate earlier
// than the rendered one or EqualsTrimmed never matches.
char s_mouseText[kTutorialMouseMsgMax][1024];
int  s_mouseTextCount = 0;
bool s_mouseTextBuilt = false;

void BuildMouseTextIfNeeded() {
    if (s_mouseTextBuilt) return;
    int count = 0;
    const MouseMsg* msgs = ActiveMouseMsgs(&count);
    int resolved = 0;
    for (int i = 0; i < count; ++i) {
        s_mouseText[i][0] = '\0';
        if (acc::engine::LookupTlk(msgs[i].strref, s_mouseText[i],
                                   sizeof(s_mouseText[i]))) {
            ++resolved;
        }
    }
    if (resolved > 0) {
        s_mouseTextBuilt = true;
        s_mouseTextCount = count;
        acclog::Write("TutorialHint", "popup mouse-text map built: %d/%d strrefs resolved",
                      resolved, count);
    }
}

const char* HintForMouseText(const char* text) {
    if (!text || !text[0]) return nullptr;
    BuildMouseTextIfNeeded();
    if (!s_mouseTextBuilt) return nullptr;
    int count = 0;
    const MouseMsg* msgs = ActiveMouseMsgs(&count);
    for (int i = 0; i < s_mouseTextCount; ++i) {
        if (s_mouseText[i][0] && EqualsTrimmed(text, s_mouseText[i])) {
            const char* s = acc::strings::Get(msgs[i].hint);
            return (s[0]) ? s : nullptr;
        }
    }
    return nullptr;
}

}  // namespace acc::tutorial_hints
