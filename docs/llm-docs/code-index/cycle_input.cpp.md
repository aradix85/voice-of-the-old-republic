# cycle_input.cpp (1053 lines)

Source: `patches/Accessibility/cycle_input.cpp`

user32.lib for GetAsyncKeyState / GetForegroundWindow / GetWindowThreadProcessId. The patch build script (create-patch.bat) links only sqlite3 by default; pragma keeps the link config local to the TU that needs it.

## Declarations (in source order)

- L79 `CategoryBindings BindingsFor( acc::filter::CycleCategory c, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`
- L114 `int FormatItemPayload(const char* name, bool haveYaw, int clock, int metres, char* outBuf, size_t outBufSize)`: Format the "{name}, {clock} o'clock, {metres} metres" payload (or the no-clock variant) into outBuf.
- L129 `void ResolvePinNoteText(void* pin, char* outBuf, size_t bufSize)`: Resolve a map pin's spoken note text exactly as AnnounceCurrent does, so the same-name comparison used for numbering keys off the string the user actually hears (note_text, or the "no text" fallback).
- L142 `void ResolveEntryName(const acc::cycle::CategoryListing& listing, int idx, bool mapHint, char* out, size_t size)`: Resolve listing entry `idx`'s spoken label, mirroring AnnounceCurrent's focused-entry cascade so the same-name comparison used for numbering keys off exactly the string the user hears.
- L185 `void AppendPositionOrdinal(const acc::cycle::CategoryListing& listing, int focusedIndex, bool mapHint, char* name, size_t nameSize)`: Append a north-to-south positional ordinal to a LISTING entry's name when two or more entries in the listing share the same spoken label.
- L227 `void AnnounceCurrent(const acc::cycle::CategoryListing& listing, const char* categoryPrefix, acc::filter::CycleContext ctx)`: Speak the locked Pillar 4 payload for whatever the cycle state currently focuses.
- L409 `void OnCycleItem(bool prev, acc::filter::CycleContext ctx)`: Step within the current category and announce the new focused item.
- L420 `void OnCycleEdge(bool last, acc::filter::CycleContext ctx)`: Jump to the first (closest) or last (farthest) item of the current category and announce it.
- L432 `void OnCycleCategory(bool prev, acc::filter::CycleContext ctx)`: Step to the next/prev non-empty category and announce "{Category}.
- L454 `acc::filter::CycleCategory ClassifyForCycle(void* obj)`: Classify a stamped object the same way cycle keys do.
- L482 `bool ResolveNarratedActivation(NarratedActivation& out)`
- L535 `bool TryResolveOrAnnounceNoFocus(NarratedActivation& a, const char* logTag)`: Wraps ResolveNarratedActivation with the shared empty-slot fallback used by `-` / Shift+- / Ctrl+- / Alt+-.
- L549 `void OnAnnounceFocus()`: `-` repeats the last narrated target with fresh distance + clock.
- L605 `acc::strings::Id GuidancePreRollFor(acc::filter::CycleCategory c)`: Per-kind pre-roll Id picker.
- L635 `bool UiOwnsKeysForWalkDispatch(const char* keyTag)`: True when a screen or popup owns the keyboard, so queueing a walk on the player creature now would be the galaxy-map Enter double-fire in another skin: one press drives the panel AND injects an action that surfaces la...
- L672 `void OnPathfindFocus()`: Shift+- — autowalk the player to the currently-focused target.
- L779 `void OnRouteFocus()`: Ctrl+- — speak the route to the currently-focused object, turn by turn, from our own A* over the static per-area nav graph (the engine refuses to plot for the leader).
- L849 `void OnPathfindFocusForce()`: Alt+- — diagnostic alternate path that bypasses the action queue via CSWSCreature::ForceMoveToPoint instead of AddMoveToPointAction.
- L894 `bool TryHandleEvent(int param_1, int param_2)`
- L985 `void PollWin32()`
- L1048 `void WalkToNarratedTarget()`
