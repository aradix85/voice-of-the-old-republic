// Internal to the minigame_pazaak module (minigame_pazaak*.cpp); no other file includes
// it. The public interface is minigame_pazaak.h. Here: what the files share.

#pragma once

#include "minigame_pazaak.h"

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "engine_reads.h"
#include "engine_offsets_select.h"

namespace acc::pazaak {

// ---- Struct offsets ------------------------------------------------------
// KOTOR 2's CSWGuiPazaakGame is the same shape with a bigger control block: the
// per-card GUI stride grows 0x31C -> 0x334 and the board gains four BTN_CHANGE
// buttons for its new dual-value cards, which pushes the whole model tail up by
// a uniform 0xF7C. Both fields are witnessed in K2's HandleContinue @0x0088F070:
// it dereferences [panel+0x964c] as the model (reading player.stand through it)
// and tests [panel+0x9650] against 3 and 4 — the same two states KOTOR 1's tests
// at +0x86d4. The 0.4f turn timer that follows lands at +0x9654, the same +4.
const size_t kPanelModelOffset = acc::off::Pick(0x86d0, 0x964C); // CSWGuiPazaakGame->pazaak (CSWPazaak*)
const size_t kPanelStateOffset = acc::off::Pick(0x86d4, 0x9650); // game_state

// CPazaakPlayer is 0x70 bytes (hand[4]=0x20 + board[9]=0x48 + stand + score).
// CSWPazaak.player is at +0x08, so enemy follows at +0x78 (NOT +0x98), and the
// per-player stand/score sit at +0x68/+0x6c — see swkotor.exe.h structs
// CPazaakPlayer / CSWPazaak.
//
// KOTOR 2 keeps this model exactly, with ONE change that cascades through all of
// it: CPazaakCard grew from 8 bytes to 12. The third dword is the resolved value
// of K2's new dual/double cards (see FormatCardLabel), and it moves every array
// stride and every field above one. CPazaakPlayer is therefore 0xa4 rather than
// 0x70 (hand 4*0xc = 0x30, board 9*0xc = 0x6c, then stand and score), and the
// deck 40*0xc.
//
// Witness for each, all in the K2 twins:
//   * player Same(+0x8) — HandleContinue forms GetTotal's `this` as model + 8.
//   * enemy +0xac — ClearGameBoard @0x008FEB00 clears the player's nine board
//     slots at +0x38 (= 0x8 + 0x30) AND the enemy's at +0xdc (= 0xac + 0x30),
//     then zeroes both stands at +0xa4 and +0x148 (= 0x8/0xac + 0x9c).
//   * cards[40] +0x150 and remaining +0x330 — ShuffleDeck @0x008FEBB0 writes
//     forty 12-byte cards from +0x150 and finishes with `[this+0x330] = 39`;
//     DrawCard @0x008FEDA0 indexes `+0x150 + n*0xc` off that same counter.
//   * hand +0x0 / board +0x30 — ChooseSidedeck @0x008FE8A0 fills four cards
//     from `this + i*0xc`; GetTotal @0x008FE830 walks nine from `this + 0x30`.
//   * stand +0x9c — read as model+0xa4 in HandleContinue and written there by
//     HandleStand; score +0xa0 follows it, which the 0xa4 player size closes.
const size_t kModelPlayerOffset = acc::off::Same(0x08);        // CSWPazaak.player (CPazaakPlayer)
const size_t kModelEnemyOffset  = acc::off::Pick(0x78, 0xAC);  // CSWPazaak.enemy
const size_t kModelRemainOffset = acc::off::Pick(0x228, 0x330); // remaining_card_count

const size_t kPlayerHandOffset  = acc::off::Same(0x00);        // CPazaakCard hand_cards[4]
const size_t kPlayerBoardOffset = acc::off::Pick(0x20, 0x30);  // CPazaakCard board_cards[9]
const size_t kPlayerStandOffset = acc::off::Pick(0x68, 0x9C);  // int stand
const size_t kPlayerScoreOffset = acc::off::Pick(0x6c, 0xA0);  // int score (sets won)

const size_t kCardIndexOffset = acc::off::Same(0x00);
const size_t kCardFlipOffset  = acc::off::Same(0x04);
// The resolved face value of K2's Double / 2&4 / 3&6 / value cards. Read only
// under IsKotor2() — on KOTOR 1 there is no such field and the offset poisons.
const size_t kCardValueOffset = acc::off::Kotor2Only(0x08);
const size_t kCardStride      = acc::off::Pick(0x08, 0x0C);

constexpr int kHandSlots  = 4;
constexpr int kBoardSlots = 9;

// KOTOR 2's five extra side-deck card indices (see FormatCardLabel). Inert on
// KOTOR 1, where 18 is already the first main-deck card — every consumer either
// gates on IsKotor2() or is only reachable from a K2 model read.
constexpr int kCardTiebreaker = 18;  // ±1 that also wins ties  ("+1T"/"-1T")
constexpr int kCardDouble     = 19;  // doubles the previous card ("D")
constexpr int kCardTwoFour    = 20;  // "2&4"
constexpr int kCardThreeSix   = 21;  // "3&6"
constexpr int kCardValue      = 22;  // value card, sign from is_flipped

// game_state values (DoGameSequence) we branch on.
constexpr int kStatePlayerInteractive = 3;  // can play a hand card + stand/end
constexpr int kStatePlayerPlayed      = 4;  // already played; stand/end only
constexpr int kStateResult            = 9;  // result sound queued
constexpr int kStateResultWait        = 10; // waiting on result message box

// `value` is CPazaakCard's third dword — KOTOR 2 only (its card is 12 bytes,
// KOTOR 1's is 8). Left at 0 on KOTOR 1, which is exactly what FormatCardLabel
// treats as "no resolved value".
struct CardView { int index; int flip; int value; };

// ---- Snapshot + delta announcements --------------------------------------
struct Snap {
    bool valid = false;
    int  state = -1;
    int  pBoard = 0, eBoard = 0;
    int  pStand = 0, eStand = 0;
    int  pScore = 0, eScore = 0;
    int  pTotal = 0, eTotal = 0;
};

extern Snap      g_prev;
extern void*     g_panel;
extern bool      g_started;
extern bool      g_resultAnnounced;
extern bool      g_boardForeground;  // board is the top foreground panel (for the input hook)

// Cursor / card-options sub-zone state. Arrow-zone model (same as the deck
// builder): Up/Down switch zones, Left/Right move within one.
constexpr int kZoneHand     = 0;   // your hand
constexpr int kZoneMyBoard  = 1;   // your played cards
constexpr int kZoneOppBoard = 2;   // the opponent's played cards
constexpr int kZoneActions  = 3;   // Stand, End turn
extern int  g_zone;
extern int  g_col;
extern bool g_optMode;
extern int  g_optSlot;
extern int  g_optSign;  // 0 = plus, 1 = minus
// Chosen magnitude for KOTOR 2's value card (index 22), whose face the player
// picks as well as its sign. The engine's own BTN_CHANGE handler @0x0088F5A0
// toggles the card's third dword between 1 and 2 and repaints, so those are the
// only two faces it has. Unused for every other card.
extern int  g_optValue;

// Helpers shared across the files, by the file that defines them.

// minigame_pazaak_model.cpp
bool IsValueChoiceCard(int index);
int FirstMainDeckIndex();
bool IsMainDeckCard(int index);
using acc::engine::TryReadI32;
void* GetModel(void* panel);
int GetState(void* panel);
void* PlayerOf(void* model);
void* EnemyOf(void* model);
CardView ReadCard(void* player, size_t baseOff, int slot);
int CallGetTotal(void* player);
int BoardCount(void* player);
CardView BoardLast(void* player, int count);
void DoPlay(void* panel, int slot);
void DoStand(void* panel);
void DoEndTurn(void* panel);

// minigame_pazaak_speech.cpp
void FormatCard(const CardView& c, bool inHand, char* out, size_t n);
void Say(const char* s, bool interrupt = false);
void SpeakHand(void* player);
void SpeakTable(void* model);
void SpeakSignOption(int index, bool interrupt);
void SpeakOppHandCount(void* model);
void SpeakBoardSide(void* model, bool playerSide, bool interrupt = true);

// minigame_pazaak_input.cpp
void ConsumeConflicts();
void PollShortcuts(void* panel);

// minigame_pazaak_wager.cpp
void ServiceWagerPopup(void* fg);

}  // namespace acc::pazaak
