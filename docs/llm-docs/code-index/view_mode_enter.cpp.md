# view_mode_enter.cpp (134 lines)

Source: `patches/Accessibility/view_mode_enter.cpp`

Enter in view mode: the deferred dispatch to the hovered object, or a walk to the cursor.

## Declarations (in source order)

- L48 `void PollEnter()`: Enter / Shift+Enter dispatch while view mode is active.
- L79 `void ProcessPendingDispatch()`
- L127 `bool ConsumedEnterThisTick()`
