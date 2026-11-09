# interact_dispatch.h (37 lines)

Source: `patches/Accessibility/interact_dispatch.h`

Enter / Shift+Enter — interact with the currently focused object.

Routes through the engine's native click pipeline (SetLastClickedOnTarget + HandleMouseClickInWorld) rather than building a CSWSObjectActionNode ourselves — the action-node format is a PlaceHolder Structure in Lane's DB and we don't need to RE it when the engine has a working "click on object" entry point that already walks + dispatches the kind-appropriate action (open door, talk, loot, pick up).

Self-gates on player loaded so menus + chargen pass Enter through.

## Declarations (in source order)

- L23 `void DispatchInteract(void* target, uint32_t handle, bool forceRadial)`: Self-gates additionally on !view_mode::IsActive — view mode owns Enter routing (cursor hover is the truth, not cycle/LastTarget).
- L35 `void InteractNarratedTarget(bool forceRadial)`: Interact with the CURRENTLY NARRATED target — the exact gesture the keyboard's Enter / Shift+Enter runs, exposed for input sources that do not go through the Win32 hotkey poll.
