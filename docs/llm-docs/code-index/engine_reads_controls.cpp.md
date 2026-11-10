# engine_reads_controls.cpp (111 lines)

Source: `patches/Accessibility/engine_reads_controls.cpp`

What a GUI control is and its state: class predicates, id, flags, list box row count, toggle state.

## Declarations (in source order)

- L11 `int ReadControlId(void* control)`
- L21 `bool IsToggle(void* control)`
- L34 `bool IsSlider(void* control)`: Vtable-identity predicates run from per-tick menu monitors.
- L44 `uint32_t ReadControlBitFlags(void* control)`
- L54 `bool IsControlVisible(void* control)`
- L58 `int ReadListBoxRowCount(void* listBox)`
- L71 `bool IsListBox(void* control)`
- L81 `bool IsEditbox(void* control)`
- L95 `bool ReadToggleState(void* toggle)`
- L99 `void DumpControlVtable(void* control, char* out, size_t outSize)`
