# interact_dispatch.cpp (560 lines)

Source: `patches/Accessibility/interact_dispatch.cpp`

Enter / Shift+Enter on the narrated target. DispatchInteract is the one pipeline every caller shares (Enter, the bridge, door walk-through, exploration).

## Declarations (in source order)

- L39 `const char* KeyName(bool forceRadial)`
- L50 `void ArmInteractApproach(const char* name, void* target, bool inputDisabled, bool isDialog, uint32_t useHandle = 0, bool leavesArea = false, const Vector* walkEnd = nullptr)`: Arm the approach tracker (guidance_approach) for an Enter dispatch: it announces "way blocked" when the walk-to-act stalls out of range and gives input back on a blocked use.
- L71 `void SpeakNoFocus(const char* why)`
- L79 `void SpeakWithName(acc::strings::Id fmt, const char* name, char* out, size_t cap)`: Speak a format string with the target name in it; `out` keeps the line for the log.
- L88 `void* ResolveInteractTarget(uint32_t* outHandle)`: The interact target is whatever was last spoken as a target name (narrated_target: passive narration, the cycle keys).
- L104 `void ClearStaleApproach(void* target)`: A re-press on the target already being walked to: that attempt is by definition not progressing (a progressing one disarms on success), and stacking another action on its queue only deepens the churn.
- L114 `void LogDispatchCreature()`: Which creature the action will run on, for correlating a leader swap with the engine action that fires.
- L132 `bool RedirectLandmarkToDoor(void* target, const char* name, bool forceRadial)`: A map note placed at a door ("Largo's Apartment" on his door) means the door: a coordinate walk stops against it closed.
- L151 `void* TransitionTriggerNear(const Vector& p)`: The transition trigger nearest to `p` within 10 m, or null: a map note such as "To Sewers" sits at the trigger it names, and a walk to it leaves the area.
- L177 `bool RedirectLandmarkToTransition(void* target, const char* name, bool forceRadial)`: A map note that stands for a way out of the area means that way out.
- L207 `bool WalkToTrigger(void* target, uint32_t handle, const char* name, acc::filter::CycleCategory cat, bool forceRadial)`: Transitions are trigger regions that fire on walk-in and landmarks are waypoints: neither has a use node, so a use would never move the PC (and report a false "way blocked").
- L247 `bool WalkThroughOpenDoor(void* target, uint32_t handle, const char* name)`: An open door has no verb, but Enter on it means going through: walk into the region beyond it.
- L289 `void AnnouncePick(const acc::picker::ActionSnapshot& snap, uint32_t handle, const char* name, acc::filter::CycleCategory cat, bool forceRadial, char* msg, size_t cap)`: The pre-roll for a picked action.
- L317 `bool UseWithInputDisabled(uint32_t handle, bool* inputDisabled)`: AddUseObjectAction with input disabled, its proven contract: on success engine_player's queue-watched session gives control back, on a stall the approach tracker does.
- L327 `bool LastUseDidNothing(uint32_t handle)`: A use the engine accepted and never acted on, on this target, the press before: a container that was bashed open takes AddUseObjectAction and does nothing with it.
- L336 `bool DispatchVerb(acc::picker::ActionSnapshot& snap, uint32_t dispatchHandle, const char* name, void* target, bool forceRadial)`: Dispatch the picked verb.
- L379 `acc::strings::Id PreRollFor(acc::filter::CycleCategory c)`
- L398 `bool ShouldSwitchFromInGameMenu()`: The in-game menu (Escape menu and its tabs, which keep the InGameMenu strip in the foreground) is the only blocker the action-menu openers switch out of, as the game's own menu hotkeys do; message boxes, dialogs and s...
- L404 `void OnInteract(bool forceRadial)`
- L426 `void DispatchInteract(void* target, uint32_t handle, bool forceRadial)`
