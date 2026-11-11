// The tutorial popup side of the tutorial hints: the keyboard hint spoken
// for a mouse-worded tutorial.2da popup, found by the popup's row or by its
// rendered message text.

#include "tutorial_hints_internal.h"

#include <windows.h>
#include <cstdint>

#include "engine_game.h"     // IsKotor2: per-game strref and row tables
#include "engine_offsets_select.h"
#include "engine_reads.h"
#include "strings.h"
#include "text_match.h"

namespace acc::tutorial_hints {

namespace {

using acc::strings::Id;

}  // namespace

int ReadTutorialRow(void* panel) {
    uint8_t row = 0;
    return acc::engine::TryReadU8(panel, kTutorialBoxRowOffset, &row) ? row : -1;
}

const char* HintForTutorialRow(int row) {
    // Rows 0-42 carry the same labels in both games' tutorial.2da, so this
    // block is shared. Rows only
    // KOTOR 2 fires — its extra rows 43+, plus rows that exist in KOTOR 1's
    // 2DA but were left unmapped there — follow in the K2-only block.
    Id id = Id::Count_;
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

namespace {

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
// message_xbox*. The engine reads message_pc on PC
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

const MouseMsg* ActiveMouseMsgs(int* count) {
    if (acc::game::IsKotor2()) {
        *count = kTutorialMouseMsgCountK2;
        return kTutorialMouseMsgsK2;
    }
    *count = kTutorialMouseMsgCountK1;
    return kTutorialMouseMsgsK1;
}

// 1024: K2's longest mapped popup text (the mine tutorial) is about 850
// bytes, and a stored copy shorter than the rendered one (kAnnounceTextMax)
// would never match.
TlkTexts<kTutorialMouseMsgMax, 1024> s_mouseText;

}  // namespace

const char* HintForMouseText(const char* text) {
    if (!text || !text[0]) return nullptr;
    int count = 0;
    const MouseMsg* msgs = ActiveMouseMsgs(&count);
    if (!s_mouseText.Ensure(count, [&](int i) { return msgs[i].strref; }, "popup mouse-text")) {
        return nullptr;
    }
    const int i = s_mouseText.Find(
        [&](const char* stored) { return acc::text::EqualsTrimmed(text, stored); });
    if (i < 0) return nullptr;
    const char* s = acc::strings::Get(msgs[i].hint);
    return s[0] ? s : nullptr;
}

}  // namespace acc::tutorial_hints
