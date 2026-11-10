// Pazaak as speech: card names, the hand, the boards and totals, and the
// sign choice of a plus-or-minus card.

#include "minigame_pazaak_internal.h"

#include "engine_game.h"
#include "speech.h"
#include "strings.h"

namespace acc::pazaak {

namespace {

// Appends a card name to a comma-separated list.
void AppendCard(char* out, size_t n, const CardView& c, bool inHand) {
    char card[96];
    FormatCard(c, inHand, card, sizeof(card));
    if (out[0]) strncat(out, ", ", n - strlen(out) - 1);
    strncat(out, card, n - strlen(out) - 1);
}

void BuildBoard(void* player, char* out, size_t n) {
    using namespace acc::strings;
    out[0] = '\0';
    int cnt = BoardCount(player);
    for (int i = 0; i < cnt; ++i) {
        AppendCard(out, n, ReadCard(player, kPlayerBoardOffset, i), false);
    }
    if (!out[0]) snprintf(out, n, "%s", Get(Id::PazaakBoardEmpty));
}

// Magnitude of a ±-faced card, for the sign chooser's spoken preview.
// 12..17 are ±1..±6; KOTOR 2's tiebreaker (18) is a ±1; its value card (22)
// carries whatever face the player has dialled in. Only ever called for indices
// IsSignChoiceCard() accepted.
int SignCardMagnitude(int index) {
    if (index >= 12 && index <= 17) return index - 11;
    if (index == kCardValue) return g_optValue;
    return 1;
}

}  // namespace

// A card's label from FormatCardLabel (docs/pazaak-investigation.md §5):
// inHand selects the Hand context (both faces + current); board cards are
// Committed.
void FormatCard(const CardView& c, bool inHand, char* out, size_t n) {
    FormatCardLabel(c.index, c.flip,
                    inHand ? CardContext::Hand : CardContext::Committed, out, n,
                    c.value);
}

void Say(const char* s, bool interrupt) {
    if (s && s[0]) acc::speech::Speak(s, interrupt);
}

void SpeakHand(void* player) {
    using namespace acc::strings;
    char line[400]; line[0] = '\0';
    int count = 0;
    for (int i = 0; i < kHandSlots; ++i) {
        CardView c = ReadCard(player, kPlayerHandOffset, i);
        if (c.index < 0) continue;
        AppendCard(line, sizeof(line), c, true);
        ++count;
    }
    if (count == 0) { Say(Get(Id::PazaakHandEmpty), true); return; }
    char msg[480];
    snprintf(msg, sizeof(msg), Get(Id::PazaakFmtHand), line);
    Say(msg, true);
}

void SpeakTable(void* model) {
    SpeakBoardSide(model, true);
    SpeakBoardSide(model, false, /*interrupt=*/false);
}

// Speak the sign chooser's current face ("plus 3" / "minus 1" / "plus 2").
// interrupt=false when it follows the chooser's own opening line, so the two
// don't cut each other off.
void SpeakSignOption(int index, bool interrupt) {
    char opt[48];
    snprintf(opt, sizeof(opt),
             acc::strings::Get(g_optSign ? acc::strings::Id::PazaakFmtMinus
                                         : acc::strings::Id::PazaakFmtPlus),
             SignCardMagnitude(index));
    Say(opt, interrupt);
}

// The opponent's remaining hand cards, count only — the same public information
// a sighted player reads off the face-down cards, never the values (Shift+C).
void SpeakOppHandCount(void* model) {
    using namespace acc::strings;
    void* e = EnemyOf(model);
    int n = 0;
    for (int i = 0; i < kHandSlots; ++i) {
        if (ReadCard(e, kPlayerHandOffset, i).index >= 0) ++n;
    }
    char msg[96];
    snprintf(msg, sizeof(msg), Get(Id::PazaakFmtOppHand), n);
    Say(msg, true);
}

// Announce one board side as a whole ("Your board: ..., total N").
void SpeakBoardSide(void* model, bool playerSide, bool interrupt) {
    using namespace acc::strings;
    void* pl = playerSide ? PlayerOf(model) : EnemyOf(model);
    char b[400], msg[480];
    BuildBoard(pl, b, sizeof(b));
    snprintf(msg, sizeof(msg),
             Get(playerSide ? Id::PazaakFmtYourBoard : Id::PazaakFmtOppBoard),
             b, CallGetTotal(pl));
    Say(msg, interrupt);
}

// Render one of the six classic ±N flip faces into `out`.
void FormatFlipFaces(int mag, int flip, CardContext ctx, char* out, size_t n) {
    using namespace acc::strings;
    if (ctx == CardContext::Hand) {
        char face[48], both[48];
        snprintf(face, sizeof(face), Get(flip ? Id::PazaakFmtMinus : Id::PazaakFmtPlus), mag);
        snprintf(both, sizeof(both), Get(Id::PazaakFmtFlipBoth), mag);
        snprintf(out, n, Get(Id::PazaakFmtFlipCurrently), both, face);
    } else if (ctx == CardContext::Collection) {
        // Deck builder: a flip card's sign isn't decided until it's played.
        snprintf(out, n, Get(Id::PazaakFmtFlipBoth), mag);
    } else {  // Committed
        snprintf(out, n, Get(flip ? Id::PazaakFmtMinus : Id::PazaakFmtPlus), mag);
    }
}

// Shared card-label synthesizer — see minigame_pazaak.h. Used by the board
// game (through FormatCard) and the side-deck builder (menus_pazaakdeck).
//
// The two games number their cards identically up to 17 and diverge after:
//
//   0..5    +1..+6                     both
//   6..11   -1..-6                     both
//   12..17  ±1..±6 (sign at play time)  both
//   18..22  ±1 T / D / 2&4 / 3&6 / value   KOTOR 2 ONLY
//   18..27  main deck 1..10            KOTOR 1
//   23..32  main deck 1..10            KOTOR 2
//
// That is not a guess: KOTOR 2's CSWGuiPazaakCard::SetCard @0x00885FE0 switches
// on the index to choose the card art — PCARDS_POS_P for 0..5, PCARDS_NEG_P for
// 6..11, PCARDS_DBLPOS_P / DBLNEG_P for 12..17, PCARDS_GOLD_P for 0x12..0x16 and
// PCARDS_GENERIC_P for 0x17..0x20 — and prints the glyphs "+1T"/"-1T", "D",
// "2&4", "3&6" for the gold five. The deck shuffler @0x008FEBB0 independently
// fills all forty main-deck cards with `i % 10 + 0x17`.
void FormatCardLabel(int index, int flip, CardContext ctx, char* out, size_t n,
                     int value) {
    using namespace acc::strings;
    if (!out || n == 0) return;
    out[0] = '\0';
    if (index < 0) {
        snprintf(out, n, "%s", Get(index == -2 ? Id::PazaakFaceDown : Id::PazaakEmpty));
        return;
    }
    if (index <= 5)  { snprintf(out, n, Get(Id::PazaakFmtPlus),  index + 1); return; }
    if (index <= 11) { snprintf(out, n, Get(Id::PazaakFmtMinus), index - 5); return; }
    if (index <= 17) { FormatFlipFaces(index - 11, flip, ctx, out, n); return; }

    if (!acc::game::IsKotor2()) {
        // 18..27 main deck, printed as its face value 1..10.
        snprintf(out, n, Get(Id::PazaakFmtPlain), index - FirstMainDeckIndex() + 1);
        return;
    }

    // ---- KOTOR 2 special cards (18..22) ----
    switch (index) {
        case kCardTiebreaker: {
            // A ±1 that also wins ties. Say it as a ±1 first — that is the part
            // that changes the arithmetic — then name what makes it special.
            char base[96];
            FormatFlipFaces(1, flip, ctx, base, sizeof(base));
            snprintf(out, n, Get(Id::PazaakFmtCardTiebreaker), base);
            return;
        }
        case kCardDouble:
        case kCardTwoFour:
        case kCardThreeSix:
        case kCardValue: {
            const Id nameId = (index == kCardDouble)  ? Id::PazaakCardDouble
                            : (index == kCardTwoFour) ? Id::PazaakCardTwoFour
                            : (index == kCardThreeSix) ? Id::PazaakCardThreeSix
                                                       : Id::PazaakCardValue;
            // Undecided (in hand or in the collection) — just the card's name.
            // Once the engine has resolved a value into the card's third dword,
            // append it, signed for the value card the same way a played flip
            // card reads.
            if (value == 0) { snprintf(out, n, "%s", Get(nameId)); return; }
            char face[48];
            const bool negative = (index == kCardValue) && flip;
            snprintf(face, sizeof(face),
                     Get(negative ? Id::PazaakFmtMinus : Id::PazaakFmtPlus), value);
            snprintf(out, n, Get(Id::PazaakFmtFlipCurrently), Get(nameId), face);
            return;
        }
        default:
            break;
    }
    // 23..32 main deck, printed as its face value 1..10.
    snprintf(out, n, Get(Id::PazaakFmtPlain), index - FirstMainDeckIndex() + 1);
}

}  // namespace acc::pazaak
