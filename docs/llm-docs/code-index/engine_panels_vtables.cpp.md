# engine_panels_vtables.cpp (109 lines)

Source: `patches/Accessibility/engine_panels_vtables.cpp`

The heap-allocated panels with no CGuiInGame slot, identified by their vtable: each is a single class written by its constructor, a per-exe constant that no .gui file can change. K1 values are symbol-table or live-probe captures, K2 values come from the RTTI census (docs/llm-docs/re/k2/k2-vtables.csv).

## Declarations (in source order)

- L14 `bool HasVtable(void* obj, uintptr_t expected)`
- L94 `PanelKind IdentifyByVtable(void* panel)`
