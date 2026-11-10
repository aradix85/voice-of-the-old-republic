# minigame_pazaak.cpp (277 lines)

Source: `patches/Accessibility/minigame_pazaak.cpp`

Pazaak minigame accessibility — see minigame_pazaak.h and docs/pazaak-investigation.md. Here: finding the board, the per-tick snapshot and its spoken deltas, and Tick.

Design (no engine detour hook): * Identify the live board panel by its vtable, or structurally — the foreground modal whose deep fields match the CSWPazaak model layout — then track it by pointer until it leaves the manager. * Read the model each tick and announce deltas. Pure state observation, so we poll rather than hook (hook-vs-poll principle). * Drive play through the engine's own CSWGuiPazaakGame handlers (HandlePlayHandCard / HandleStand / HandleContinue), which guard on game_state internally — safe to call off our OnUpdate tick (the same deferred callback site the menu pending-queue drains from). * Input is an arrow-zone navigator (same model as the deck builder): zone 0 your hand, 1 your board, 2 opponent board, 3 actions. Up/Down switch zones, Left/Right move within, Enter plays/activates. Arrows + Enter are handled through the manager input hook (TryHandleInput) and consumed, so the generic chain can't also drive the board's buttons. The letter shortcuts (s/e/c/t, Shift+C) are Win32-polled because the engine drops those scancodes before the manager hook.

## Declarations (in source order)

- L75 `bool LooksLikePazaak(void* panel)`: Structural identity probe — true iff `panel` looks like a CSWGuiPazaakGame.
- L100 `Snap ReadSnap(void* panel)`
- L119 `void AnnounceDeltas(void* panel, const Snap& cur)`
- L198 `void ResetState()`
- L214 `bool IsBoardForeground()`
- L216 `void Tick()`
