# pad_input_dispatch.cpp (245 lines)

Source: `patches/Accessibility/pad_input_dispatch.cpp`

Gamepad presses the mod dispatches itself (KOTOR 1, and KOTOR 2 when its engine never saw the pad): engine actions and menu keys sent as scancodes, the D-Pad, A, B and the shoulders, and the D-Pad hold that jumps to the end of a run.

## Declarations (in source order)

- L33 `void InjectMenuKey(int logical)`: Navigate an ENGINE panel: send the arrow / Enter / Esc the keyboard would have sent.
- L48 `int DpadSlotForCode(int code)`
- L61 `void FireGameAction(int actionId, int defaultDik, const char* what)`: Fire a [Keymapping] game action by synthesising the key it is bound to.
- L70 `int LogicalForDirection(int code)`: The logical code a pad D-Pad / stick direction stands in for.
- L88 `void DispatchDpad(int code)`
- L111 `void DispatchA()`
- L131 `void DispatchB()`
- L146 `void DispatchShoulder(bool right)`
- L195 `bool DpadHoldSpent(int code)`: True while this direction's hold has already spent its jump.
- L202 `void DispatchDpadHold(int code)`: Pay the hold.
