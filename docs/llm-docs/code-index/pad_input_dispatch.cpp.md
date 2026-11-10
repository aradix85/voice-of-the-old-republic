# pad_input_dispatch.cpp (211 lines)

Source: `patches/Accessibility/pad_input_dispatch.cpp`

Gamepad presses the mod dispatches itself (KOTOR 1, and KOTOR 2 when its engine never saw the pad): engine actions and menu keys sent as scancodes, the D-Pad, A, B and the shoulders, and the D-Pad hold that jumps to the end of a run.

## Declarations (in source order)

- L31 `void InjectMenuKey(int logical)`: Navigate an ENGINE panel: send the arrow / Enter / Esc the keyboard would have sent.
- L46 `int DpadSlotForCode(int code)`
- L59 `void FireGameAction(int actionId, int defaultDik, const char* what)`: Fire a [Keymapping] game action by synthesising the key it is bound to.
- L68 `int LogicalForDirection(int code)`: The logical code a pad D-Pad / stick direction stands in for.
- L86 `void DispatchDpad(int code)`
- L109 `void DispatchA()`
- L113 `void DispatchB()`
- L128 `void DispatchShoulder(bool right)`
- L161 `bool DpadHoldSpent(int code)`: True while this direction's hold has already spent its jump.
- L168 `void DispatchDpadHold(int code)`: Pay the hold.
