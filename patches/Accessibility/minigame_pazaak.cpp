// Pazaak minigame accessibility — see minigame_pazaak.h and
// docs/pazaak-investigation.md. Here: finding the board, the per-tick
// snapshot and its spoken deltas, and Tick.
//
// Design (no engine detour hook):
//   * Identify the live board panel by its vtable, or structurally — the
//     foreground modal whose deep fields match the CSWPazaak model layout —
//     then track it by pointer until it leaves the manager.
//   * Read the model each tick and announce deltas. Pure state observation,
//     so we poll rather than hook (hook-vs-poll principle).
//   * Drive play through the engine's own CSWGuiPazaakGame handlers
//     (HandlePlayHandCard / HandleStand / HandleContinue), which guard on
//     game_state internally — safe to call off our OnUpdate tick (the same
//     deferred callback site the menu pending-queue drains from).
//   * Input is an arrow-zone navigator (same model as the deck builder):
//     zone 0 your hand, 1 your board, 2 opponent board, 3 actions. Up/Down
//     switch zones, Left/Right move within, Enter plays/activates. Arrows +
//     Enter are handled through the manager input hook (TryHandleInput) and
//     consumed, so the generic chain can't also drive the board's buttons.
//     The letter shortcuts (s/e/c/t, Shift+C) are Win32-polled because the
//     engine drops those scancodes before the manager hook.

#include "minigame_pazaak_internal.h"

#include "engine_game.h"
#include "engine_manager.h"
#include "engine_reads.h"
#include "engine_panels.h"
#include "engine_addr.h"
#include "log.h"
#include "strings.h"

namespace acc::pazaak {

namespace {

// "Tutorial active" gate. DoGameSequence is only pumped while the board is the
// topmost panel (Draw's IsOnTop gate, see docs/pazaak-investigation.md §7); the
// tutorial game's ShowHelp stacks an inaccessible help message-box ON TOP of the
// board (at the first draw, the opponent's turn, etc.), which drops the board
// off-top and freezes the turn engine until a sighted player dismisses it.
// ShowHelp and the End-Turn/Stand nags all no-op when this flag is 0, so we
// clear it on acquire to disable the visual tutorial outright — our narration
// replaces it.
//
// The two games spell the same gate differently, which is why this is a Pick of
// a whole address rather than one struct's field:
//   * KOTOR 1 reads it INSIDE the embedded CSWGuiTutorial — panel+0x7d20 is the
//     sub-panel, +0x994 its own active flag.
//   * KOTOR 2 hoists it to a plain panel field at +0x9630, set from the
//     constructor's third argument (showTutorial). Its HandleContinue /
//     HandleStand / HandlePlayHandCard each gate their ShowHelp on
//     `[panel+0x9630] != 0` before calling it on the CSWGuiTutorial that now
//     lives at panel+0x8c58. Clearing the field disables all three the same way.
const size_t kTutorialActiveOffset = acc::off::Pick(0x7d20 + 0x994, 0x9630);

// CSWGuiPazaakGame's own vtable. KOTOR 1's came out of Lane's symbol export;
// KOTOR 2's out of its RTTI class map. The board is a heap-allocated modal with
// no CGuiInGame slot, so vtable equality is the cleanest identifier there is —
// this is the same shape as every other single-instance panel in engine_panels.
// The structural probe below stays as the fallback: it is what found the board
// before the vtable was known, it costs nothing while untracked, and it keeps
// working if a build relocates the table.
const uintptr_t kVtableCSWGuiPazaakGame = acc::addr::Pick(0x00753358, 0x009A6404);

// Highest card index the engine can put in a hand slot. KOTOR 1 tops out at 27
// (main deck 18..27); KOTOR 2 inserts five special cards at 18..22 and pushes
// its main deck to 23..32 — see FormatCardLabel.
constexpr int kMaxCardIndexK1 = 27;
constexpr int kMaxCardIndexK2 = 32;

// Structural identity probe — true iff `panel` looks like a CSWGuiPazaakGame.
// All reads SEH-guarded: a smaller panel faults on the deep field reads and
// returns false. The nested model-field range checks make a false positive
// from adjacent heap effectively impossible.
bool LooksLikePazaak(void* panel) {
    const int maxCard =
        acc::game::IsKotor2() ? kMaxCardIndexK2 : kMaxCardIndexK1;
    __try {
        if (acc::engine::HasVtable(panel, kVtableCSWGuiPazaakGame)) return true;
        auto* p = reinterpret_cast<unsigned char*>(panel);
        void* model = *reinterpret_cast<void**>(p + kPanelModelOffset);
        if (!model) return false;
        int state = *reinterpret_cast<int*>(p + kPanelStateOffset);
        if (state < 0 || state > 0xc) return false;
        auto* m = reinterpret_cast<unsigned char*>(model);
        int pScore = *reinterpret_cast<int*>(m + kModelPlayerOffset + kPlayerScoreOffset);
        int eScore = *reinterpret_cast<int*>(m + kModelEnemyOffset  + kPlayerScoreOffset);
        int remain = *reinterpret_cast<int*>(m + kModelRemainOffset);
        int hand0  = *reinterpret_cast<int*>(m + kModelPlayerOffset + kPlayerHandOffset);
        if (pScore < 0 || pScore > 3) return false;
        if (eScore < 0 || eScore > 3) return false;
        if (remain < -1 || remain > 40) return false;
        if (hand0 < -2 || hand0 > maxCard) return false;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

Snap ReadSnap(void* panel) {
    Snap s;
    void* model = GetModel(panel);
    if (!model) return s;
    void* p = PlayerOf(model);
    void* e = EnemyOf(model);
    s.state  = GetState(panel);
    s.pBoard = BoardCount(p);
    s.eBoard = BoardCount(e);
    TryReadI32(p, kPlayerStandOffset, &s.pStand);
    TryReadI32(e, kPlayerStandOffset, &s.eStand);
    TryReadI32(p, kPlayerScoreOffset, &s.pScore);
    TryReadI32(e, kPlayerScoreOffset, &s.eScore);
    s.pTotal = CallGetTotal(p);
    s.eTotal = CallGetTotal(e);
    s.valid = true;
    return s;
}

void AnnounceDeltas(void* panel, const Snap& cur) {
    using namespace acc::strings;
    if (!g_prev.valid) return;  // first valid snapshot is baseline only
    void* model = GetModel(panel);
    if (!model) return;
    void* p = PlayerOf(model);
    void* e = EnemyOf(model);
    char card[96], msg[200];

    // Player auto-drew a main-deck card. Side cards reach the board via our
    // Play handler, which speaks its own confirmation — so gate the draw line
    // to main-deck indices (18+ on KOTOR 1, 23+ on KOTOR 2, whose five extra
    // side-deck cards occupy 18..22 — see IsMainDeckCard).
    if (cur.pBoard > g_prev.pBoard) {
        CardView lc = BoardLast(p, cur.pBoard);
        if (IsMainDeckCard(lc.index)) {
            FormatCard(lc, false, card, sizeof(card));
            snprintf(msg, sizeof(msg), Get(Id::PazaakFmtYouDrew), card, cur.pTotal);
            Say(msg);
            if (cur.pTotal > 20) Say(Get(Id::PazaakOverTwenty));
        }
    }

    // Opponent board grew: drew a main-deck card, or played a side card.
    if (cur.eBoard > g_prev.eBoard) {
        CardView lc = BoardLast(e, cur.eBoard);
        if (IsMainDeckCard(lc.index)) {
            // Main-deck draws are face-up/public, so name the card like the
            // player's draw line (not just the running total).
            FormatCard(lc, false, card, sizeof(card));
            snprintf(msg, sizeof(msg), Get(Id::PazaakFmtOppDrew), card, cur.eTotal);
            Say(msg);
        } else if (lc.index >= 0) {
            FormatCard(lc, false, card, sizeof(card));
            snprintf(msg, sizeof(msg), Get(Id::PazaakFmtOppPlayed), card, cur.eTotal);
            Say(msg);
        }
    }

    // Stands.
    if (cur.eStand && !g_prev.eStand) {
        snprintf(msg, sizeof(msg), Get(Id::PazaakFmtOppStands), cur.eTotal);
        Say(msg);
    }
    if (cur.pStand && !g_prev.pStand) {
        snprintf(msg, sizeof(msg), Get(Id::PazaakFmtYouStand), cur.pTotal);
        Say(msg);
    }

    // Turn start — entered the interactive player state.
    if (cur.state == kStatePlayerInteractive && g_prev.state != kStatePlayerInteractive) {
        Say(Get(Id::PazaakYourTurn));
    }

    // Set / match results — latched once per set (cleared on board reset).
    if (!g_resultAnnounced) {
        if (cur.pScore > g_prev.pScore) {
            if (cur.pScore >= 3) Say(Get(Id::PazaakWinMatch), true);
            else {
                snprintf(msg, sizeof(msg), Get(Id::PazaakFmtWinSet), cur.pScore, cur.eScore);
                Say(msg);
            }
            g_resultAnnounced = true;
        } else if (cur.eScore > g_prev.eScore) {
            if (cur.eScore >= 3) Say(Get(Id::PazaakLoseMatch), true);
            else {
                snprintf(msg, sizeof(msg), Get(Id::PazaakFmtLoseSet), cur.pScore, cur.eScore);
                Say(msg);
            }
            g_resultAnnounced = true;
        } else if ((cur.state == kStateResult || cur.state == kStateResultWait) &&
                   g_prev.state != kStateResult && g_prev.state != kStateResultWait) {
            // Reached the result phase with no score change → a tie set.
            Say(Get(Id::PazaakTieReplay), true);
            g_resultAnnounced = true;
        }
    }
}

void ResetState() {
    g_panel = nullptr;
    g_prev = Snap{};
    g_zone = kZoneHand;
    g_col = 0;
    g_optMode = false;
    g_optSlot = -1;
    g_optSign = 0;
    g_optValue = 1;
    g_started = false;
    g_resultAnnounced = false;
    g_boardForeground = false;
}

}  // namespace

bool IsBoardForeground() { return g_boardForeground; }

void Tick() {
    void* mgr = acc::engine::GetGuiManager();
    if (!mgr) { if (g_panel) ResetState(); return; }

    void* fg = acc::engine::GetForegroundPanel(mgr);

    // The wager popup precedes (and is a different panel from) the board.
    ServiceWagerPopup(fg);

    // Drop the tracked panel once it leaves the manager (game ended / panel
    // destroyed). IsPanelInManager is deref-free, so it's safe on a stale
    // pointer.
    if (g_panel && !acc::engine::IsPanelInManager(g_panel)) {
        acclog::Write("Pazaak", "board panel %p left manager; game ended", g_panel);
        ResetState();
    }

    // Acquire the board panel: an unknown-kind foreground modal whose deep
    // fields match the model layout. Only probed while untracked, so the
    // structural read isn't paid during normal play.
    if (!g_panel && fg &&
        acc::engine::IdentifyPanel(fg) == acc::engine::PanelKind::Unknown &&
        LooksLikePazaak(fg)) {
        ResetState();
        g_panel = fg;
        // Disable the visual tutorial so its help popups can't stack on top of
        // the board and freeze the turn engine (see kTutorialActiveOffset).
        WriteIntAt(fg, kTutorialActiveOffset, 0);
        acclog::Write("Pazaak", "acquired board panel=%p", fg);
    }
    if (!g_panel) return;

    // One-time orientation cue.
    if (!g_started) {
        g_started = true;
        Say(acc::strings::Get(acc::strings::Id::PazaakStart), true);
    }

    // Snapshot + announce — runs even while a result message box is layered
    // above the board (that's exactly when win/lose/tie lands).
    Snap cur = ReadSnap(g_panel);
    if (cur.valid) {
        if (g_prev.valid && cur.pBoard < g_prev.pBoard) {
            // New set dealt (board cleared) — re-arm the result latch + cursor.
            g_resultAnnounced = false;
            g_zone = kZoneHand;
            g_col = 0;
        }
        AnnounceDeltas(g_panel, cur);
        g_prev = cur;
    }

    // Input — only when the board itself is the active foreground panel.
    g_boardForeground = (fg == g_panel);
    if (g_boardForeground) {
        ConsumeConflicts();
        PollShortcuts(g_panel);
    }
}

}  // namespace acc::pazaak
