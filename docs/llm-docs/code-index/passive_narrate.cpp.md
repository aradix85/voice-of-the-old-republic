# passive_narrate.cpp (496 lines)

Source: `patches/Accessibility/passive_narrate.cpp`

(From passive_narrate.h.) Passive-selection narration.

Hooks CClientExoAppInternal::ShowObject — the engine's single point of user-facing target change (called by DoPassiveSelection's mouse-hover auto-target and SelectNearestObject's Q/E cycle). On a real transition the watcher resolves the handle, classifies it through the six-category nav filter, and speaks the localised name plus a 3D positional cue.

Hook ShowObject rather than poll GetLastTarget because last_target is also written by combat AI (CSWSCreature::CreateNewAttackActions, every round, every queued creature) and per-tick polling races that. ShowObject is only called by user-driven paths, so it's a clean signal.

Does NOT handle focus-loss announce (silent on sentinel transitions), the Q/E cycle itself (cycle_input owns that), or filtering to "interesting" — the engine already decided by calling ShowObject; we just drop non-nav categories.

## Declarations (in source order)

- L87 `acc::filter::CycleCategory ClassifyForNarration(void* obj)`: Returns Count_ for non-nav consumers (combat / dialog target).
- L102 `bool IsActivePartyMember(void* obj)`: True when obj is one of the active party followers.
- L124 `bool NarrateHandle(uint32_t handle, const char* reason, bool explicitRequest)`: Resolve → classify → speak → stamp.
- L257 `void OnEngineShowObject(uint32_t handle)`
- L358 `void RequestQEReannounce(int directionCode)`
- L372 `bool IsInSynthesizedQE()`
- L376 `void Tick()`
- L453 `bool NarrateExplicit(uint32_t serverHandle)`
- L463 `void __cdecl OnShowObject(void* /*clientObject*/, int handle)`
- L479 `void __cdecl OnShowObjectK2(void* ebp)`
