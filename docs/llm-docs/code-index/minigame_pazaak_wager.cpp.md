# minigame_pazaak_wager.cpp (126 lines)

Source: `patches/Accessibility/minigame_pazaak_wager.cpp`

The wager popup before a match: the amount spoken as it changes, and Left and Right held down to step it faster.

## Declarations (in source order)

- L48 `void StepWager(void* fg, int dir)`
- L52 `void SpeakWager(int cur, int max)`
- L62 `void ServiceWagerPopup(void* fg)`: Run every tick while the wager popup may be foreground (from Tick).
