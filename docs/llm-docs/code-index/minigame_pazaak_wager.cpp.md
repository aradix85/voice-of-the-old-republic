# minigame_pazaak_wager.cpp (125 lines)

Source: `patches/Accessibility/minigame_pazaak_wager.cpp`

The wager popup before a match: the amount spoken as it changes, and Left and Right held down to step it faster.

## Declarations (in source order)

- L48 `void StepWager(void* fg, int dir)`
- L56 `void ServiceWagerPopup(void* fg)`: Run every tick while the wager popup may be foreground (from Tick).
