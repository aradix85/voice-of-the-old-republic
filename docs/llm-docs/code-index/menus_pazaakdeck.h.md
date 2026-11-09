# menus_pazaakdeck.h (50 lines)

Source: `patches/Accessibility/menus_pazaakdeck.h`

Pazaak side-deck builder (CSWGuiPazaakStart) — the pre-game screen where the player picks 10 cards from their collection for the match's side deck.

The screen's 28 card slots are CSWGuiPazaakCard widgets (vtable 0x007531c0) that carry no text of their own, so the generic chain reads them as blank "unlabeled buttons". This module labels them from the panel's card model (card_counts[18] + sidedeck[10]) and filters the decorative overlay labels.

Layout (verified against the live panel dump + struct header): all_cards[18]    @ +0x1A4   (available-card grid, stride 0x31C) sidedeck_gui[10] @ +0x501C  (chosen-deck slot widgets, stride 0x31C) card_counts[18]  @ +0x755C  (int — copies of each type still available) sidedeck[10]     @ +0x75A4  (CPazaakCard — the chosen cards)

## Declarations (in source order)

- L22 `bool IsDeckPanel(void* panel)`: True iff `panel` is the CSWGuiPazaakStart side-deck builder (vtable match).
- L29 `bool ExtractCardLabel(void* panel, void* control, char* outBuf, size_t bufSize)`: Synthesize speech for a CSWGuiPazaakCard widget on the deck panel: available grid → "{card}, N available" / "{card}, none left" chosen slot → "Deck slot K: {card}" / "Deck slot K: empty" Returns false (outBuf untouche...
- L35 `bool IsChainDecorative(void* panel, void* control)`: Chain filter: drop the overlay value/count/section-title labels and the unaddable (zero-owned) available cards, so navigation steps cleanly through the addable cards, the 10 chosen slots, and the Play button.
- L42 `bool TryHandleInput(void* panel, int param_1, int param_2, int& rv)`: 3-row arrow navigator (row 0 = collection, row 1 = the 10 deck slots, row 2 = controls/Play).
- L47 `void Tick()`: Drains the add / remove / play action staged by TryHandleInput (deferred so deep engine calls stay off the input-dispatch stack).
