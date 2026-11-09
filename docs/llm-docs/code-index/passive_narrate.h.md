# passive_narrate.h (57 lines)

Source: `patches/Accessibility/passive_narrate.h`

Passive-selection narration.

Hooks CClientExoAppInternal::ShowObject — the engine's single point of user-facing target change (called by DoPassiveSelection's mouse-hover auto-target and SelectNearestObject's Q/E cycle). On a real transition the watcher resolves the handle, classifies it through the six-category nav filter, and speaks the localised name plus a 3D positional cue.

Hook ShowObject rather than poll GetLastTarget because last_target is also written by combat AI (CSWSCreature::CreateNewAttackActions, every round, every queued creature) and per-tick polling races that. ShowObject is only called by user-driven paths, so it's a clean signal.

Does NOT handle focus-loss announce (silent on sentinel transitions), the Q/E cycle itself (cycle_input owns that), or filtering to "interesting" — the engine already decided by calling ShowObject; we just drop non-nav categories.

## Declarations (in source order)

- L27 `void OnEngineShowObject(uint32_t handle)`: ShowObject hook entry.
- L38 `void RequestQEReannounce(int directionCode)`: Arm a Q/E re-announce.
- L44 `bool IsInSynthesizedQE()`: Returns true while Tick is mid-flight on a synthesized Q/E re-issue.
- L51 `bool NarrateExplicit(uint32_t serverHandle)`: Speak and stamp a server object exactly like an explicit Q/E landing on it: name and combat brief, clock and distance, narrated target, discovery.
- L54 `void Tick()`: Per-tick drain of pending Q/E re-announce + sentinel-skip retry.
