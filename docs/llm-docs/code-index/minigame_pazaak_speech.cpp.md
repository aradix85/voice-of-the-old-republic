# minigame_pazaak_speech.cpp (207 lines)

Source: `patches/Accessibility/minigame_pazaak_speech.cpp`

Pazaak as speech: card names, the hand, the boards and totals, and the sign choice of a plus-or-minus card.

## Declarations (in source order)

- L15 `void AppendCard(char* out, size_t n, const CardView& c, bool inHand)`: Appends a card name to a comma-separated list.
- L22 `void BuildBoard(void* player, char* out, size_t n)`
- L36 `int SignCardMagnitude(int index)`: Magnitude of a ±-faced card, for the sign chooser's spoken preview.
- L47 `void FormatCard(const CardView& c, bool inHand, char* out, size_t n)`: A card's label from FormatCardLabel (docs/pazaak-investigation.md §5): inHand selects the Hand context (both faces + current); board cards are Committed.
- L53 `void Say(const char* s, bool interrupt)`
- L57 `void SpeakHand(void* player)`
- L73 `void SpeakTable(void* model)`
- L81 `void SpeakSignOption(int index, bool interrupt)`: Speak the sign chooser's current face ("plus 3" / "minus 1" / "plus 2").
- L92 `void SpeakOppHandCount(void* model)`: The opponent's remaining hand cards, count only — the same public information a sighted player reads off the face-down cards, never the values (Shift+C).
- L105 `void SpeakBoardSide(void* model, bool playerSide, bool interrupt)`: Announce one board side as a whole ("Your board: ..., total N").
- L117 `void FormatFlipFaces(int mag, int flip, CardContext ctx, char* out, size_t n)`: Render one of the six classic ±N flip faces into `out`.
- L150 `void FormatCardLabel(int index, int flip, CardContext ctx, char* out, size_t n, int value)`: Shared card-label synthesizer — see minigame_pazaak.h.
