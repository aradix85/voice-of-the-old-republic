# minigame_pazaak_speech.cpp (215 lines)

Source: `patches/Accessibility/minigame_pazaak_speech.cpp`

Pazaak as speech: card names, the hand, the boards and totals, and the sign choice of a plus-or-minus card.

## Declarations (in source order)

- L14 `void BuildBoard(void* player, char* out, size_t n)`: ---- On-demand readouts (c / t) ------------------------------------------
- L32 `int SignCardMagnitude(int index)`: Magnitude of a ±-faced card, for the sign chooser's spoken preview.
- L44 `void FormatCard(const CardView& c, bool inHand, char* out, size_t n)`: ---- Card label synthesis (docs/pazaak-investigation.md §5) --------------- Thin wrapper over the shared synthesizer (acc::pazaak::FormatCardLabel, defined below the anonymous namespace and declared in pazaak.h).
- L50 `void Say(const char* s, bool interrupt)`
- L54 `void SpeakHand(void* player)`
- L73 `void SpeakTable(void* model)`
- L89 `void SpeakSignOption(int index, bool interrupt)`: Speak the sign chooser's current face ("plus 3" / "minus 1" / "plus 2").
- L100 `void SpeakOppHandCount(void* model)`: The opponent's remaining hand cards, count only — the same public information a sighted player reads off the face-down cards, never the values (Shift+C).
- L113 `void SpeakBoardSide(void* model, bool playerSide)`: Announce one board side as a whole ("Your board: ..., total N").
- L125 `void FormatFlipFaces(int mag, int flip, CardContext ctx, char* out, size_t n)`: Render one of the six classic ±N flip faces into `out`.
- L158 `void FormatCardLabel(int index, int flip, CardContext ctx, char* out, size_t n, int value)`: Shared card-label synthesizer — see pazaak.h.
