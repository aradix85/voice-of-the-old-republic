# minigame_pazaak.h (78 lines)

Source: `patches/Accessibility/minigame_pazaak.h`

Pazaak minigame accessibility (CSWGuiPazaakGame board).

Per-tick driver: locate the live Pazaak board panel, announce state deltas (draws, plays, stands, set/match results) and poll keyboard play. No-op when no board is active. Wired into core_tick::Dispatch ahead of the in-world / menu pollers so it can Consume() the shared keys (Tab / Enter / arrows / Esc) before those pollers sample them.

See docs/pazaak-investigation.md for the full reverse-engineering.

## Declarations (in source order)

- L23 `enum class CardContext`: Card-label context for FormatCardLabel: Committed — a board card with a decided sign (flip cards show their face).
- L35 `void FormatCardLabel(int index, int flip, CardContext ctx, char* out, size_t n, int value = 0)`: Synthesize a localized card label from the card index (see §5 of docs/pazaak-investigation.md) into `out`.
- L42 `bool IsSignChoiceCard(int index)`: True iff `index` is one of KOTOR 2's ±-faced cards, i.e.
- L47 `bool IsBoardForeground()`: True iff the Pazaak board is the current foreground panel (updated each Tick).
- L52 `bool TryHandleInput(void* activePanel, int param_1, int param_2, int& rv)`: Manager-hook input handler for the board's arrow-zone navigator.
- L73 `void DispatchWagerInput(void* panel, int code)`: Drive a wager-popup less/more step by calling CSWGuiWagerPopup:: HandleInputEvent(panel, code, 1) directly — the same path the engine's own button-push callbacks take.
- L75 `void Tick()`

Also 2 constants or variables at namespace scope.
