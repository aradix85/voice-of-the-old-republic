# view_mode_enter.cpp (151 lines)

Source: `patches/Accessibility/view_mode_enter.cpp`

Enter in view mode: the deferred dispatch to the hovered object, or a walk to the cursor.

## Declarations (in source order)

- L52 `void PollEnter()`: Enter / Shift+Enter dispatch while view mode is active.
- L93 `void ProcessPendingDispatch()`: Runs at the top of Tick BEFORE the IsActive() gate — by design, this fires after PollEnter exited view mode synchronously.
- L144 `bool ConsumedEnterThisTick()`
