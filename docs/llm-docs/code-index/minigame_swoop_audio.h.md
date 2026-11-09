# minigame_swoop_audio.h (41 lines)

Source: `patches/Accessibility/minigame_swoop_audio.h`

Swoop race spatial-audio: continuous obstacle + accelerator-pad proximity cues. Walks the global CSWMiniGameObjectArray each tick, classifies each slot via the AsObstacle / AsEnemy vtable downcasts, drives one LoopSource per in-range obstacle (multi-source warn pulse) and one nearest-only LoopSource for accelerator pads (clean booster channel). See swoop_race.cpp for the race lifecycle, gear watch, and wall-collision detection that drives this module via Tick().

## Declarations (in source order)

- L18 `void TickSpatialAudio(void* miniGame)`: Drive obstacle + accelpad proximity loops for the active race.
- L23 `void ResetSpatialAudio()`: Stop every active loop and clear the per-race diagnostic guards.
- L37 `enum class ParkedBar`: KOTOR 2: what the parked bike is stuck against, as seen by the obstacle sweep (gear 0, inside a low bar's lateral span, the bar just ahead).
- L38 `ParkedBar GetParkedBarSituation()`
