# cycle_input.cpp (210 lines)

Source: `patches/Accessibility/cycle_input.cpp`

Cycle input: the two ingestion paths (the engine key hook and the Win32 poll) and the category table both the announce and the focus handlers use.

## Declarations (in source order)

- L26 `CategoryBindings BindingsFor( acc::filter::CycleCategory c, acc::filter::CycleContext ctx)`
- L60 `bool TryHandleEvent(int param_1, int param_2)`
- L146 `void PollWin32()`
