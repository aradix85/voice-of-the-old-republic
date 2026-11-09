# input_poll_router.cpp (560 lines)

Source: `patches/Accessibility/input_poll_router.cpp`

per-tick Win32 hotkey router - see input_poll_router.h for what this is and why it is not merged into input_pipeline.cpp.

Split out of interact_hotkey.cpp (now interact_dispatch.cpp) by the Phase-1 structure pass (refactoring candidate 19). The function body is verbatim; only the namespace changed, from acc::interact to acc::input_poll, because routing for a dozen subsystems was never interact-specific.

The gate ORDER below is behaviour. Do not reorder without an in-game input pass.

## Declarations (in source order)

- L48 `void PollHotkey()`
