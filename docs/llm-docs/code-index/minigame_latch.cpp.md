# minigame_latch.cpp (33 lines)

Source: `patches/Accessibility/minigame_latch.cpp`

(From minigame_latch.h.) Which minigame of ours is running. The swoop race and the turret game are both a CSWMiniGame that the player's area points at; each module latches the struct at entry and keeps it through the moments the area stops pointing at it. Engine-free: minigame_engine reads the two truth sources.

## Declarations (in source order)

- L5 `LatchStep StepLatch(Latch& latch, const LatchInput& in)`
