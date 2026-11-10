# interact_dispatch.h (36 lines)

Source: `patches/Accessibility/interact_dispatch.h`

Enter / Shift+Enter — interact with the currently focused object.

Routes through the engine's native click pipeline (SetLastClickedOnTarget + HandleMouseClickInWorld) rather than building a CSWSObjectActionNode ourselves — the action-node format is a PlaceHolder Structure in Lane's DB and we don't need to RE it when the engine has a working "click on object" entry point that already walks + dispatches the kind-appropriate action (open door, talk, loot, pick up).

Self-gates on player loaded so menus + chargen pass Enter through.

## Declarations (in source order)

- L23 `acc::strings::Id PreRollFor(acc::filter::CycleCategory c)`: The verb before a target's name for its kind ("Open X", "Talk to X", "Pick up X"); Enter and the Shift+- walk speak the same verbs.
- L33 `void DispatchInteract(void* target, uint32_t handle, bool forceRadial)`: Self-gates additionally on !view_mode::IsActive — view mode owns Enter routing (cursor hover is the truth, not cycle/LastTarget).
