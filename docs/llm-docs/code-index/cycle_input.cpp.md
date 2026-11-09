# cycle_input.cpp (1165 lines)

Source: `patches/Accessibility/cycle_input.cpp`

user32.lib for GetAsyncKeyState / GetForegroundWindow / GetWindowThreadProcessId. The patch build script (create-patch.bat) links only sqlite3 by default; pragma keeps the link config local to the TU that needs it.

## Declarations (in source order)

- L91 `acc::audio::NavCue RefineDoorCue(acc::audio::NavCue cue, void* obj)`: Refine a placeholder door cue against the actual server-door open_state and material.
- L102 `CategoryBindings BindingsFor( acc::filter::CycleCategory c, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`
- L142 `int FormatItemPayload(const char* name, bool haveYaw, int clock, int metres, char* outBuf, size_t outBufSize)`: Format the "{name}, {clock} o'clock, {metres} metres" payload (or the no-clock variant) into outBuf.
- L157 `void ResolvePinNoteText(void* pin, char* outBuf, size_t bufSize)`: Resolve a map pin's spoken note text exactly as AnnounceCurrent does, so the same-name comparison used for numbering keys off the string the user actually hears (note_text, or the "no text" fallback).
- L170 `void ResolveEntryName(const acc::cycle::CategoryListing& listing, int idx, bool mapHint, char* out, size_t size)`: Resolve listing entry `idx`'s spoken label, mirroring AnnounceCurrent's focused-entry cascade so the same-name comparison used for numbering keys off exactly the string the user hears.
- L213 `void AppendPositionOrdinal(const acc::cycle::CategoryListing& listing, int focusedIndex, bool mapHint, char* name, size_t nameSize)`: Append a north-to-south positional ordinal to a LISTING entry's name when two or more entries in the listing share the same spoken label.
- L255 `void AnnounceCurrent(const acc::cycle::CategoryListing& listing, const char* categoryPrefix, acc::filter::CycleContext ctx)`: Speak the locked Pillar 4 payload for whatever the cycle state currently focuses.
- L447 `void OnCycleItem(bool prev, acc::filter::CycleContext ctx)`: Step within the current category and announce the new focused item.
- L458 `void OnCycleEdge(bool last, acc::filter::CycleContext ctx)`: Jump to the first (closest) or last (farthest) item of the current category and announce it.
- L470 `void OnCycleCategory(bool prev, acc::filter::CycleContext ctx)`: Step to the next/prev non-empty category and announce "{Category}.
- L492 `acc::filter::CycleCategory ClassifyForCycle(void* obj)`: Classify a stamped object the same way cycle keys do.
- L520 `bool ResolveNarratedActivation(NarratedActivation& out)`
- L573 `bool TryResolveOrAnnounceNoFocus(NarratedActivation& a, const char* logTag)`: Wraps ResolveNarratedActivation with the shared empty-slot fallback used by `-` / Shift+- / Ctrl+- / Alt+-.
- L586 `void PlayActivationCue(const NarratedActivation& a)`: Per-category 3D cue at the target — spatial confirmation of WHERE the action is aimed.
- L598 `void OnAnnounceFocus()`: `-` repeats the last narrated target with fresh distance + clock.
- L656 `acc::strings::Id GuidancePreRollFor(acc::filter::CycleCategory c)`: Per-kind pre-roll Id picker.
- L686 `bool UiOwnsKeysForWalkDispatch(const char* keyTag)`: True when a screen or popup owns the keyboard, so queueing a walk on the player creature now would be the galaxy-map Enter double-fire in another skin: one press drives the panel AND injects an action that surfaces la...
- L723 `void OnPathfindFocus()`: Shift+- — autowalk the player to the currently-focused target.
- L845 `void OnBeaconFocus()`: Ctrl+- — start an audio beacon to the currently-focused Pillar 4 object via our own A* over the static per-area nav graph (engine refuses to plot for the leader).
- L931 `void OnPathfindFocusForce()`: Alt+- — diagnostic alternate path that bypasses the action queue via CSWSCreature::ForceMoveToPoint instead of AddMoveToPointAction.
- L978 `bool TryHandleEvent(int param_1, int param_2)`
- L1069 `bool DispatchPadAction(PadAction action)`
- L1097 `void PollWin32()`
- L1160 `void WalkToNarratedTarget()`
