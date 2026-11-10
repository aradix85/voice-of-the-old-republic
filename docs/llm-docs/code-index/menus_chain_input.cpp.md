# menus_chain_input.cpp (218 lines)

Source: `patches/Accessibility/menus_chain_input.cpp`

Input against an already-built chain (menus_chain.cpp builds it): the Up/Down/Home/End step with its announcement and cursor warp, and the child walker. Enter is in menus_chain_input_enter.cpp, Left/Right and Esc in menus_chain_input_keys.cpp; menus_dispatch.cpp decides the handler order.

## Declarations (in source order)

- L34 `void OnEmptyChain(void* panel, bool& consumed)`: A panel with nothing to navigate is left to the engine, but walked into the log once so its structure can be studied (the engine never focuses these, so the usual panel walk does not fire).
- L63 `void SpeakCountSuffix(acc::strings::Id fmt, int n)`
- L71 `void AnnounceStep(const ChainEntry& e)`: Speak the newly focused entry, then the per-panel extras.
- L103 `void WarpCursor(const ChainEntry& e, bool safeToDrive, int* x, int* y)`: Warp the cursor onto the entry so the engine's hover handler follows the focus.
- L125 `void WalkChildren(const char* label, void* parent, size_t offset, const char* kindName)`
- L179 `void HandleNavStep(void* activePanel, int code, int val, bool& consumed)`
