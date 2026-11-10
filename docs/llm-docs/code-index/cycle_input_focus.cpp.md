# cycle_input_focus.cpp (437 lines)

Source: `patches/Accessibility/cycle_input_focus.cpp`

Acting on the narrated target: re-announcing it, and walking, routing or pathfinding to it.

## Declarations (in source order)

- L45 `bool ResolveNarratedActivation(NarratedActivation& out)`
- L98 `bool TryResolveOrAnnounceNoFocus(NarratedActivation& a, const char* logTag)`: Wraps ResolveNarratedActivation with the shared empty-slot fallback used by `-` / Shift+- / Ctrl+- / Alt+-.
- L121 `bool UiOwnsKeysForWalkDispatch(const char* keyTag)`: True when a screen or popup owns the keyboard, so queueing a walk on the player creature now would be the galaxy-map Enter double-fire in another skin: one press drives the panel AND injects an action that surfaces la...
- L142 `void OnAnnounceFocus()`: `-` repeats the last narrated target with fresh distance + clock.
- L212 `void OnPathfindFocus()`: Shift+- — autowalk the player to the currently-focused target.
- L319 `void OnRouteFocus()`: Ctrl+- — speak the route to the currently-focused object, turn by turn, from our own A* over the static per-area nav graph (the engine refuses to plot for the leader).
- L389 `void OnPathfindFocusForce()`: Alt+- — diagnostic alternate path that bypasses the action queue via CSWSCreature::ForceMoveToPoint instead of AddMoveToPointAction.
- L432 `void WalkToNarratedTarget()`
