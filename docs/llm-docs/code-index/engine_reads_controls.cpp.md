# engine_reads_controls.cpp (123 lines)

Source: `patches/Accessibility/engine_reads_controls.cpp`

What a GUI control is and its state: class predicates, id, flags, list box row count, toggle state. Every read is guarded: a non-null control can be freed (a panel teardown leaves stale pointers in monitor state for a tick), and a stale one must read as "not this class", not crash.

## Declarations (in source order)

- L14 `int ReadControlId(void* control)`
- L19 `bool IsToggle(void* control)`
- L23 `bool IsSlider(void* control)`
- L27 `bool IsListBox(void* control)`
- L33 `bool IsEditbox(void* control)`: CSWGuiSaveGameEditBox (the save-name popup) is a subclass with its own vtable and the same layout.
- L38 `uint32_t ReadControlBitFlags(void* control)`
- L44 `bool IsControlVisible(void* control)`
- L48 `int ReadListBoxRowCount(void* listBox)`
- L61 `bool ReadListBoxRowArray(void* listBox, void** outData)`: The controls array's data pointer; false when unreadable or null.
- L69 `void* ReadListBoxRow(void* listBox, int index)`
- L80 `int ReadListBoxSelection(void* listBox)`
- L86 `void* ReadListBoxSelectedRow(void* listBox)`
- L90 `int FindListBoxRow(void* listBox, void* row)`
- L105 `bool ReadToggleState(void* toggle)`
- L111 `void DumpControlVtable(void* control, char* out, size_t outSize)`
