# interact_dispatch.h (33 lines)

Source: `patches/Accessibility/interact_dispatch.h`

Enter / Shift+Enter — interact with the currently focused object.

Routes through the engine's native click pipeline (SetLastClickedOnTarget + HandleMouseClickInWorld) rather than building a CSWSObjectActionNode ourselves — the action-node format is a PlaceHolder Structure in Lane's DB and we don't need to RE it when the engine has a working "click on object" entry point that already walks + dispatches the kind-appropriate action (open door, talk, loot, pick up).

Self-gates on player loaded so menus + chargen pass Enter through.

## Declarations (in source order)

- L24 `acc::strings::Id PreRollFor(acc::filter::CycleCategory c)`: The line for a target of this kind, with its name: the verb ("Open X", "Talk to X"), or for an item in the world that it cannot be picked up.
- L30 `void DispatchInteract(void* target, uint32_t handle, bool forceRadial)`: The one interact pipeline.
