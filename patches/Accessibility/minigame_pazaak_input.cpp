// The pazaak keys: moving between the hand and the boards, playing a card,
// standing, ending the turn, and the shortcut keys.

#include "minigame_pazaak_internal.h"

#include "engine_input.h"
#include "hotkeys.h"
#include "log.h"
#include "strings.h"

namespace acc::pazaak {

namespace {

// Play the focused hand card and speak the confirmation. For a +/- card the
// caller sets the hand card's is_flipped first (mirrors the engine's own
// HandleFlipHandCard-then-play flow); UsePlayerSidedeckCard copies the
// 8-byte CPazaakCard, sign included, onto the board.
void PlayAndAnnounce(void* panel, void* player, int slot) {
    using namespace acc::strings;
    CardView c = ReadCard(player, kPlayerHandOffset, slot);
    char card[96];
    FormatCard(c, false, card, sizeof(card)); // committed sign
    DoPlay(panel, slot);
    char msg[200];
    snprintf(msg, sizeof(msg), Get(Id::PazaakFmtYouPlayed), card, CallGetTotal(player));
    Say(msg, true);
    acclog::Write("Pazaak", "play slot=%d index=%d flip=%d", slot, c.index, c.flip);
}

// The board's two turn actions, with their one turn gate. Two surfaces reach
// them — the S / E shortcuts and Enter in the actions zone — and the gate
// has to answer the same way on both, including the spoken
// refusal: a press that lands out of turn must say so, not go quiet.
void ActStand(void* panel, int state) {
    using namespace acc::strings;
    if (state == kStatePlayerInteractive || state == kStatePlayerPlayed) {
        DoStand(panel);
    } else {
        Say(Get(Id::PazaakNotYourTurn), true);
    }
}

void ActEndTurn(void* panel, int state) {
    using namespace acc::strings;
    if (state == kStatePlayerInteractive || state == kStatePlayerPlayed) {
        DoEndTurn(panel);
        Say(Get(Id::PazaakTurnEnded), true);
    } else {
        Say(Get(Id::PazaakNotYourTurn), true);
    }
}

// Closes the sign chooser of a plus-or-minus card.
void CloseSignChooser() {
    g_optMode = false;
    g_optSlot = -1;
}

void SpeakAction(int col) {
    using namespace acc::strings;
    Say(Get(col == 0 ? Id::PazaakStandLabel : Id::PazaakEndTurnLabel), true);
}

int ZoneLen(void* model, int zone) {
    if (zone == kZoneHand)    return kHandSlots;
    if (zone == kZoneMyBoard) return BoardCount(PlayerOf(model));
    if (zone == kZoneOppBoard) return BoardCount(EnemyOf(model));
    return 2;                                          // Stand, End turn
}

// Hand slots can be empty (index < 0) when a card has been played or the hand
// holds fewer than four cards. Left/Right and zone entry skip those slots so
// the user only ever lands on a playable card.
bool HandSlotFilled(void* player, int slot) {
    if (slot < 0 || slot >= kHandSlots) return false;
    return ReadCard(player, kPlayerHandOffset, slot).index >= 0;
}

int FirstHandCol(void* player) {
    for (int i = 0; i < kHandSlots; ++i)
        if (HandSlotFilled(player, i)) return i;
    return 0;  // hand empty — stay at slot 0 (entry summary says "hand empty")
}

// Next filled slot in `dir` (+1 right / -1 left); returns `from` if none (no wrap).
int NextHandCol(void* player, int from, int dir) {
    for (int c = from + dir; c >= 0 && c < kHandSlots; c += dir)
        if (HandSlotFilled(player, c)) return c;
    return from;
}

// Up/Down entered a zone — read its summary so the user gets oriented.
void AnnounceZoneEntry(void* model, int zone) {
    if (zone == kZoneHand)          SpeakHand(PlayerOf(model));
    else if (zone == kZoneMyBoard)  SpeakBoardSide(model, true);
    else if (zone == kZoneOppBoard) SpeakBoardSide(model, false);
    else                            SpeakAction(g_col);
}

// Left/Right moved within a zone — read the single focused element.
void AnnounceZoneElement(void* model, int zone, int col) {
    using namespace acc::strings;
    char card[96];
    if (zone == kZoneHand) {
        CardView c = ReadCard(PlayerOf(model), kPlayerHandOffset, col);
        FormatCard(c, true, card, sizeof(card));
        Say(card, true);
    } else if (zone == kZoneMyBoard || zone == kZoneOppBoard) {
        void* pl = (zone == kZoneMyBoard) ? PlayerOf(model) : EnemyOf(model);
        int cnt = BoardCount(pl);
        if (cnt == 0) { Say(Get(Id::PazaakBoardEmpty), true); return; }
        if (col >= cnt) col = cnt - 1;
        CardView c = ReadCard(pl, kPlayerBoardOffset, col);
        FormatCard(c, false, card, sizeof(card));
        Say(card, true);
    } else {
        SpeakAction(col);
    }
}

// Enter on the focused zone element (driven from TryHandleInput).
void HandleEnter(void* panel, void* model, int state) {
    using namespace acc::strings;
    if (g_zone == kZoneHand) {
        if (state != kStatePlayerInteractive) {
            Say(Get(state == kStatePlayerPlayed ? Id::PazaakNoPlayable
                                                : Id::PazaakNotYourTurn), true);
            return;
        }
        void* player = PlayerOf(model);
        CardView c = ReadCard(player, kPlayerHandOffset, g_col);
        if (c.index < 0) { Say(Get(Id::PazaakNoPlayable), true); return; }
        if (IsSignChoiceCard(c.index)) {
            // +/- card → sign sub-zone, default to the plus face. KOTOR 2's
            // value card also carries a magnitude the player picks; adopt
            // whatever face it currently shows so the first Up/Down is a real
            // change rather than a no-op re-announcement.
            g_optMode = true; g_optSlot = g_col; g_optSign = 0;
            g_optValue = (c.value == 2) ? 2 : 1;
            Say(Get(IsValueChoiceCard(c.index) ? Id::PazaakChooseSignAndValue
                                               : Id::PazaakChooseSign), true);
            SpeakSignOption(c.index, /*interrupt=*/false);
        } else {
            PlayAndAnnounce(panel, player, g_col);
        }
    } else if (g_zone == kZoneActions) {
        if (g_col == 0) ActStand(panel, state);
        else            ActEndTurn(panel, state);
    }
    // The boards are read-only: Enter does nothing there.
}

}  // namespace

// Suppress the in-world / menu actions that share our physical keys so their
// pollers (which run later in Dispatch) don't act on the same press. Per the
// Claim-vs-Consume rule, Consume works here because we run inside the same
// BeginTick/EndTick window as those pollers.
void ConsumeConflicts() {
    using namespace acc::hotkeys;
    Consume(Action::PartyLeaderAnnounce);  // Tab
    Consume(Action::InteractTarget);       // Enter
    Consume(Action::InteractForceRadial);  // Shift+Enter
    Consume(Action::NavUp);
    Consume(Action::NavDown);
    Consume(Action::NavLeft);
    Consume(Action::NavRight);
    Consume(Action::SubmenuEsc);           // Esc
    Consume(Action::ContainerGiveMode);    // Q/E
    Consume(Action::StoreModeToggle);      // Q/E
}

// Win32-polled shortcuts — letter scancodes the engine drops before the
// manager hook, so we read them directly (mirrors the in-world cycle keys).
// Arrow + Enter navigation goes through TryHandleInput (the manager hook).
void PollShortcuts(void* panel) {
    using namespace acc::hotkeys;
    using namespace acc::strings;
    void* model = GetModel(panel);
    if (!model) return;
    void* player = PlayerOf(model);
    int state = GetState(panel);

    if (Pressed(Action::PazaakReviewHand))  SpeakHand(player);
    if (Pressed(Action::PazaakReviewTable)) SpeakTable(model);

    // Shift+C — opponent's remaining hand cards (count only — the same public
    // info a sighted player reads off the face-down cards, never the values).
    if (Pressed(Action::PazaakOppHand)) SpeakOppHandCount(model);

    // s / e quick shortcuts for Stand / End turn (also reachable in the
    // actions zone via arrows + Enter).
    if (Pressed(Action::PazaakStand))   ActStand(panel, state);
    if (Pressed(Action::PazaakEndTurn)) ActEndTurn(panel, state);
}

// Arrow + Enter navigator, driven from the manager input hook so the generic
// chain can't also act on the board's controls. Returns true (+ rv) when it
// consumes the key. Letters fall through to PollShortcuts; Esc (outside the
// sub-zone) falls through to the engine's quit-confirm.
bool TryHandleInput(void* /*activePanel*/, int param_1, int param_2, int& rv) {
    // g_boardForeground is set by Tick(), which only arms once it has acquired
    // a real board panel, so this stays inert everywhere else on its own.
    if (!g_boardForeground || !g_panel) return false;
    void* panel = g_panel;

    const bool isNav   = (param_1 == kInputNavUp   || param_1 == kInputNavDown ||
                          param_1 == kInputNavLeft || param_1 == kInputNavRight);
    const bool isEnter = (param_1 == kInputEnter1  || param_1 == kInputEnter2);
    const bool isEsc   = (param_1 == kInputEsc1    || param_1 == kInputEsc2);

    // Card-options sub-zone (a +/- card mid-play) owns the arrows, Enter, Esc.
    // Left/Right pick the sign; on KOTOR 2's value card Up/Down also pick the
    // face (1 or 2), mirroring what its BTN_CHANGE does.
    if (g_optMode) {
        if (!isNav && !isEnter && !isEsc) return false;
        rv = 1;
        if (param_2 == 0) return true;
        void* model  = GetModel(panel);
        void* player = model ? PlayerOf(model) : nullptr;
        if (param_1 == kInputNavLeft || param_1 == kInputNavRight) {
            g_optSign ^= 1;
            if (player) {
                SpeakSignOption(ReadCard(player, kPlayerHandOffset, g_optSlot).index,
                                /*interrupt=*/true);
            }
        } else if (param_1 == kInputNavUp || param_1 == kInputNavDown) {
            if (player) {
                CardView c = ReadCard(player, kPlayerHandOffset, g_optSlot);
                // Only the value card has a second face; on every other card
                // this is a deliberate no-op that still re-reads the current
                // one, so Up/Down never lands on silence.
                if (IsValueChoiceCard(c.index)) g_optValue = (g_optValue == 1) ? 2 : 1;
                SpeakSignOption(c.index, /*interrupt=*/true);
            }
        } else if (isEnter) {
            if (player) {
                auto* card = reinterpret_cast<unsigned char*>(player) +
                             kPlayerHandOffset + (size_t)g_optSlot * kCardStride;
                CardView c = ReadCard(player, kPlayerHandOffset, g_optSlot);
                WriteIntAt(card, kCardFlipOffset, g_optSign);
                // The value card's face lives in the third dword — the same
                // field the engine's own BTN_CHANGE writes.
                if (IsValueChoiceCard(c.index)) {
                    WriteIntAt(card, kCardValueOffset, g_optValue);
                }
                int slot = g_optSlot;
                CloseSignChooser();
                PlayAndAnnounce(panel, player, slot);
            } else {
                CloseSignChooser();
            }
        } else {  // Esc cancels the sign chooser without playing.
            CloseSignChooser();
            Say(acc::strings::Get(acc::strings::Id::PazaakCancelled), true);
        }
        return true;
    }

    if (!isNav && !isEnter) return false;
    rv = 1;
    if (param_2 == 0) return true;

    void* model = GetModel(panel);
    if (!model) return true;
    int state = GetState(panel);

    if (isNav) {
        if (param_1 == kInputNavUp || param_1 == kInputNavDown) {
            const int dir = (param_1 == kInputNavUp) ? -1 : 1;
            if ((dir < 0 && g_zone > kZoneHand) || (dir > 0 && g_zone < kZoneActions)) {
                g_zone += dir;
                // Land on the first filled hand slot when entering the hand;
                // other zones have no gaps, so just clamp.
                if (g_zone == kZoneHand)
                    g_col = FirstHandCol(PlayerOf(model));
                else if (g_col >= ZoneLen(model, g_zone))
                    g_col = 0;
                AnnounceZoneEntry(model, g_zone);
            }
        } else if (param_1 == kInputNavLeft || param_1 == kInputNavRight) {
            const int dir = (param_1 == kInputNavLeft) ? -1 : 1;
            int next;
            if (g_zone == kZoneHand) {
                next = NextHandCol(PlayerOf(model), g_col, dir);  // skip empty slots
            } else {
                next = g_col + dir;
                const int len = ZoneLen(model, g_zone);
                if (next < 0 || next >= len) next = g_col;
            }
            if (next != g_col) { g_col = next; AnnounceZoneElement(model, g_zone, g_col); }
        }
        return true;
    }

    HandleEnter(panel, model, state);  // Enter
    return true;
}

}  // namespace acc::pazaak
