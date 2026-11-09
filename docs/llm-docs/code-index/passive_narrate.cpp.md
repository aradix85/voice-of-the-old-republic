# passive_narrate.cpp (537 lines)

Source: `patches/Accessibility/passive_narrate.cpp`

(From passive_narrate.h.) Passive-selection narration.

Hooks CClientExoAppInternal::ShowObject — the engine's single point of user-facing target change (called by DoPassiveSelection's mouse-hover auto-target and SelectNearestObject's Q/E cycle). On a real transition the watcher resolves the handle, classifies it through the six-category nav filter, and speaks the localised name plus a 3D positional cue.

Hook ShowObject rather than poll GetLastTarget because last_target is also written by combat AI (CSWSCreature::CreateNewAttackActions, every round, every queued creature) and per-tick polling races that. ShowObject is only called by user-driven paths, so it's a clean signal.

Does NOT handle focus-loss announce (silent on sentinel transitions), the Q/E cycle itself (cycle_input owns that), or filtering to "interesting" — the engine already decided by calling ShowObject; we just drop non-nav categories.

## Declarations (in source order)

- L90 `acc::audio::NavCue ClosedDoorCueForMaterial(void* obj)`: Mirrors cycle_input.cpp's mapping.
- L101 `acc::audio::NavCue CueForCategory(acc::filter::CycleCategory c, void* obj)`: Door cue depends on open_state + material.
- L119 `acc::filter::CycleCategory ClassifyForNarration(void* obj)`: Returns Count_ for non-nav consumers (combat / dialog target).
- L134 `bool IsActivePartyMember(void* obj)`: True when obj is one of the active party followers.
- L156 `bool NarrateHandle(uint32_t handle, const char* reason, bool explicitRequest)`: Resolve → classify → speak → stamp.
- L298 `void OnEngineShowObject(uint32_t handle)`
- L399 `void RequestQEReannounce(int directionCode)`
- L413 `bool IsInSynthesizedQE()`
- L417 `void Tick()`
- L494 `bool NarrateExplicit(uint32_t serverHandle)`
- L504 `void __cdecl OnShowObject(void* /*clientObject*/, int handle)`
- L520 `void __cdecl OnShowObjectK2(void* ebp)`
