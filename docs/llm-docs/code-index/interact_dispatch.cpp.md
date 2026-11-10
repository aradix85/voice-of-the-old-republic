# interact_dispatch.cpp (779 lines)

Source: `patches/Accessibility/interact_dispatch.cpp`

overlay Esc-close so the engine-event consume guard wins the poll-vs-event race

## Declarations (in source order)

- L49 `void ArmInteractApproach(const char* name, void* target, bool inputDisabled, bool isDialog, uint32_t useHandle = 0)`: Arm the unified approach tracker for an Enter-interact dispatch.
- L77 `bool ShouldSwitchFromInGameMenu()`
- L86 `acc::strings::Id PreRollFor(acc::filter::CycleCategory c)`: Pick the per-kind pre-roll string.
- L105 `acc::filter::CycleCategory ClassifyForInteract(void* obj)`: Classify like passive_narrate does — first matching category wins.
- L131 `void* ResolveInteractTarget(uint32_t* outHandle)`: Resolve the "what does the user want to interact with" target.
- L160 `void OnInteract(bool forceRadial)`
- L196 `void DispatchInteractImpl(void* target, uint32_t handle, bool forceRadial)`
- L636 `void AnnounceBarePersonalKey(int slot)`
- L723 `void AnnounceBareTargetKey(int row)`
- L774 `void DispatchInteract(void* target, uint32_t handle, bool forceRadial)`: Public seam introduced 2026-05-06 (lay-off 5).
