# menus_pazaakdeck.cpp (396 lines)

Source: `patches/Accessibility/menus_pazaakdeck.cpp`

Pazaak side-deck builder accessibility — see menus_pazaakdeck.h.

Integrates with the menu chain: ExtractCardLabel plugs into the per-kind extractor ladder (menus_extract.cpp) so focused card widgets announce a real label, and IsChainDecorative plugs into RebindChain's decorative filter (menus_chain.cpp) so the overlay labels and unaddable cards drop out of navigation. Card identity comes from the panel's card model (card_counts / sidedeck), not the widgets — AddChosenCard writes the model but doesn't call SetCard on the chosen-slot widgets.

## Declarations (in source order)

- L61 `int NumTypes()`: Collectable card types: 18 on KOTOR 1 (6 plus, 6 minus, 6 flip), 24 on KOTOR 2 (those plus its five specials, with the grid's last cell unused).
- L64 `bool VtableIs(void* p, uintptr_t vt)`
- L69 `int ReadIntAt(void* base, size_t off)`
- L77 `bool Classify(void* panel, void* control, bool& outAvailable, int& outIdx)`: Classify a card widget by its position in the panel's embedded arrays.
- L97 `bool IsDeckPanel(void* panel)`
- L101 `bool ExtractCardLabel(void* panel, void* control, char* outBuf, size_t bufSize)`
- L135 `bool IsChainDecorative(void* panel, void* control)`
- L192 `int BuildCollection(void* panel, int* outTypes)`: Owned card types, ascending.
- L208 `int DeckFilledCount(void* panel)`
- L215 `int RowLength(void* panel, int row)`
- L225 `void Speak(const char* s)`
- L227 `void AnnounceFocus(void* panel)`
- L280 `bool TryHandleInput(void* panel, int param_1, int param_2, int& rv)`
- L331 `void Tick()`
