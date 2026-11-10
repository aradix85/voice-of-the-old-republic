# interact_dispatch.cpp (501 lines)

Source: `patches/Accessibility/interact_dispatch.cpp`

Enter / Shift+Enter on the narrated target, and the announcements of the bare action-bar keys. DispatchInteract is the one pipeline every caller shares (Enter, view mode, the bridge, door walk-through, exploration).

## Declarations (in source order)

- L39 `const char* KeyName(bool forceRadial)`
- L47 `void ArmInteractApproach(const char* name, void* target, bool inputDisabled, bool isDialog, uint32_t useHandle = 0)`: Arm the approach tracker (guidance_approach) for an Enter dispatch: it announces "way blocked" when the walk-to-act stalls out of range and gives input back on a blocked use.
- L63 `acc::strings::Id PreRollFor(acc::filter::CycleCategory c)`: The action verb that goes before the target name, per kind.
- L80 `acc::filter::CycleCategory ClassifyForInteract(void* obj)`: First matching category, like passive_narrate; Count_ when none matches (the interact still runs, without a localised pre-roll).
- L89 `void SpeakNoFocus(const char* why)`
- L97 `void SpeakWithName(acc::strings::Id fmt, const char* name, char* out, size_t cap)`: Speak a format string with the target name in it; `out` keeps the line for the log.
- L106 `void* ResolveInteractTarget(uint32_t* outHandle)`: The interact target is whatever was last spoken as a target name (narrated_target: passive narration, the cycle keys, view-mode hover).
- L122 `void ClearStaleApproach(void* target)`: A re-press on the target already being walked to: that attempt is by definition not progressing (a progressing one disarms on success), and stacking another action on its queue only deepens the churn.
- L132 `void LogDispatchCreature()`: Which creature the action will run on, for correlating a leader swap with the engine action that fires.
- L150 `bool RedirectLandmarkToDoor(void* target, const char* name, bool forceRadial)`: A map note placed at a door ("Largo's Apartment" on his door) means the door: a coordinate walk stops against it closed.
- L170 `bool WalkToTrigger(void* target, uint32_t handle, const char* name, acc::filter::CycleCategory cat, bool forceRadial)`: Transitions are trigger regions that fire on walk-in and landmarks are waypoints: neither has a use node, so a use would never move the PC (and report a false "way blocked").
- L190 `bool WalkThroughOpenDoor(void* target, uint32_t handle, const char* name)`: An open door has no verb, but Enter on it means going through: walk to it.
- L207 `void AnnouncePick(const acc::picker::ActionSnapshot& snap, uint32_t handle, const char* name, acc::filter::CycleCategory cat, bool forceRadial, char* msg, size_t cap)`: The pre-roll for a picked action.
- L235 `bool UseWithInputDisabled(uint32_t handle, bool* inputDisabled)`: AddUseObjectAction with input disabled, its proven contract: on success engine_player's queue-watched session gives control back, on a stall the approach tracker does.
- L246 `bool DispatchVerb(acc::picker::ActionSnapshot& snap, uint32_t dispatchHandle, const char* name, void* target, bool forceRadial)`: Dispatch the picked verb.
- L280 `void SpeakColumnEmpty(int index, char* msg, size_t cap)`: The bare action-bar keys: the empty-column phrase, 1-based for the user.
- L292 `bool ShouldSwitchFromInGameMenu()`: The in-game menu (Escape menu and its tabs, which keep the InGameMenu strip in the foreground) is the only blocker the action-menu openers switch out of, as the game's own menu hotkeys do; message boxes, dialogs and s...
- L298 `void OnInteract(bool forceRadial)`
- L320 `void DispatchInteract(void* target, uint32_t handle, bool forceRadial)`
- L436 `void AnnounceBarePersonalKey(int slot)`: Bare keys 4-7 fire personal action-bar columns 0-3 in the engine itself (no hook on DoPersonalAction).
- L474 `void AnnounceBareTargetKey(int row)`: Bare keys 1-3: the target-action rows 0-2, the same way.
