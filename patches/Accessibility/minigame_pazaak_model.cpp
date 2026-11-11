// The pazaak game as the engine holds it: reading the panel, the model, the
// players and their cards, and calling the engine's own play, stand,
// end-turn and wager handlers.

#include "minigame_pazaak_internal.h"

#include "engine_game.h"
#include "engine_addr.h"
#include "log.h"

namespace acc::pazaak {

namespace {

// ---- Engine surfaces (docs/pazaak-investigation.md §13) -------------------
// HandleContinue / HandleStand are __thiscall(this, CSWGuiControl* param_1) —
// param_1 is the originating button, null when invoked programmatically. Their
// first guard is `param_1 == 0 || param_1->is_active != 0`, so the typedef must
// carry the argument and we pass null; leaving it out hands the guard stack
// garbage to dereference.
typedef int  (__thiscall* PFN_GetTotal)(void* player);
typedef void (__thiscall* PFN_HandleCtrl)(void* panel, void* control);
typedef void (__thiscall* PFN_HandleInt)(void* panel, int index);

// KOTOR 2 twins, all four reached from its CSWGuiPazaakGame constructor
// @0x0088aac0 and its own ctor-registered handlers, then body-matched against
// KOTOR 1's:
//   * HandleContinue @0x0088F070 — registered on BTN_XTEXT (panel+0x7b10) for
//     event 0x27. Same guard chain (`param_1 == 0 || param_1->is_active`, player
//     not stood, state 3 or 4), same ShowHelp strrefs (0x96f1/0x7e4c/0x96e2),
//     same tail (state = 5, timer = 0.4f, RefreshDisplay).
//   * HandleStand @0x0088F180 — BTN_YTEXT (panel+0x7cf0). Distinguished from
//     HandleContinue by the one line that matters: it WRITES player.stand = 1.
//   * HandlePlayHandCard @0x0088F280 — not registered directly; the hand-card
//     entry's own event-0x27 handler @0x0088EE30 calls it with the card
//     control's custom_value (the slot) as its single int argument, exactly
//     KOTOR 1's shape. Same guards (state == 3, slot 0..3, CanUse...).
//   * GetTotal @0x008FE830 — witnessed at HandleContinue's call site, where the
//     `this` is formed as `model + 8` (i.e. CSWPazaak.player); the body walks
//     nine board slots at +0x30 with a 12-byte stride.
const uintptr_t kAddrGetTotal = acc::addr::Pick(0x006e4360, 0x008FE830); // CPazaakPlayer::GetTotal
const uintptr_t kAddrHandleContinue = acc::addr::Pick(0x0067ec20, 0x0088F070); // End Turn
const uintptr_t kAddrHandleStand = acc::addr::Pick(0x0067ed00, 0x0088F180); // Stand
const uintptr_t kAddrHandlePlayHandCard = acc::addr::Pick(0x0067ede0, 0x0088F280); // Play hand card (int slot)
const uintptr_t kAddrWagerHandleInput = acc::addr::Pick(0x0067e150, 0x00887B10); // CSWGuiWagerPopup::HandleInputEvent
typedef void (__thiscall* PFN_WagerHandleInput)(void* popup, int code, int state);

bool ReadPtrAt(void* base, size_t off, void** out) {
    __try {
        *out = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(base) + off);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

}  // namespace

// True for the one card whose FACE the player picks as well as its sign.
bool IsValueChoiceCard(int index) {
    return acc::game::IsKotor2() && index == kCardValue;
}

// First main-deck index: 18 on KOTOR 1, 23 on KOTOR 2 (whose five special
// side-deck cards take 18..22).
int FirstMainDeckIndex() { return acc::game::IsKotor2() ? 23 : 18; }

bool IsMainDeckCard(int index) { return index >= FirstMainDeckIndex(); }

// ---- SEH-guarded primitive reads -----------------------------------------
bool ReadIntAt(void* base, size_t off, int* out) {
    __try {
        *out = *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(base) + off);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

void WriteIntAt(void* base, size_t off, int v) {
    __try {
        *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(base) + off) = v;
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void* GetModel(void* panel) {
    void* m = nullptr;
    return ReadPtrAt(panel, kPanelModelOffset, &m) ? m : nullptr;
}

int GetState(void* panel) {
    int s = -1;
    return ReadIntAt(panel, kPanelStateOffset, &s) ? s : -1;
}

void* PlayerOf(void* model) {
    return reinterpret_cast<unsigned char*>(model) + kModelPlayerOffset;
}

void* EnemyOf(void* model) {
    return reinterpret_cast<unsigned char*>(model) + kModelEnemyOffset;
}

CardView ReadCard(void* player, size_t baseOff, int slot) {
    CardView c{ -1, 0, 0 };
    void* cardPtr = reinterpret_cast<unsigned char*>(player) + baseOff + (size_t)slot * kCardStride;
    ReadIntAt(cardPtr, kCardIndexOffset, &c.index);
    ReadIntAt(cardPtr, kCardFlipOffset, &c.flip);
    if (acc::game::IsKotor2()) ReadIntAt(cardPtr, kCardValueOffset, &c.value);
    return c;
}

int CallGetTotal(void* player) {
    __try {
        auto fn = reinterpret_cast<PFN_GetTotal>(kAddrGetTotal);
        return fn(player);
    } __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

int BoardCount(void* player) {
    for (int i = 0; i < kBoardSlots; ++i) {
        if (ReadCard(player, kPlayerBoardOffset, i).index == -1) return i;
    }
    return kBoardSlots;
}

CardView BoardLast(void* player, int count) {
    if (count <= 0) return CardView{ -1, 0, 0 };
    return ReadCard(player, kPlayerBoardOffset, count - 1);
}

// ---- Engine action dispatch (thiscall, this = panel) ----------------------
void DoPlay(void* panel, int slot) {
    __try { reinterpret_cast<PFN_HandleInt>(kAddrHandlePlayHandCard)(panel, slot); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void DoStand(void* panel) {
    __try { reinterpret_cast<PFN_HandleCtrl>(kAddrHandleStand)(panel, nullptr); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void DoEndTurn(void* panel) {
    __try { reinterpret_cast<PFN_HandleCtrl>(kAddrHandleContinue)(panel, nullptr); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

bool IsSignChoiceCard(int index) {
    if (index >= 12 && index <= 17) return true;                 // ±1..±6, both games
    return acc::game::IsKotor2() && index == kCardTiebreaker;    // ±1 T, KOTOR 2 only
}

void DispatchWagerInput(void* panel, int code) {
    if (!panel) return;
    __try {
        auto fn = reinterpret_cast<PFN_WagerHandleInput>(kAddrWagerHandleInput);
        fn(panel, code, 1);
        acclog::Write("Pazaak", "wager HandleInputEvent panel=%p code=0x%x",
                      panel, code);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Pazaak", "wager HandleInputEvent SEH panel=%p code=0x%x",
                      panel, code);
    }
}

}  // namespace acc::pazaak
