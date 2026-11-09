# menus_chain_input.cpp (825 lines)

Source: `patches/Accessibility/menus_chain_input.cpp`

chain-navigation input handlers.

Split out of menus_chain.cpp by the Phase-1 structure pass (refactoring candidate 2). menus_chain.cpp builds and maintains the chain (RebindChain and its validation/helper cluster); this file consumes input against an already-built chain. All five entry points were already declared individually in menus_chain.h, so the move is verbatim - no header change, no call-site change.

HandleEnterActivation  Enter / Shift+Enter on the chain cursor WalkChildren           child-list walker (also the empty-chain probe) HandleNavStep          Up / Down chain step + cursor warp HandleLeftRight        Left / Right (cycle widgets, sliders, tabs) HandleEsc              Esc back-out / modal close

Handler ORDER is decided by the caller in menus_dispatch.cpp, not here.

## Declarations (in source order)

- L56 `void HandleEnterActivation(void* activePanel, int code, int val, bool& consumed)`
- L368 `void WalkChildren(const char* label, void* parent, size_t offset, const char* kindName)`
- L428 `void HandleNavStep(void* activePanel, int code, int val, bool& consumed)`
- L613 `void HandleLeftRight(void* activePanel, int code, int val, bool& consumed)`
- L690 `void HandleEsc(void* activePanel, int code, int val, bool& consumed)`
