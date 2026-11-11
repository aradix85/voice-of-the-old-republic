# engine_reads_controls.cpp (135 lines)

Source: `patches/Accessibility/engine_reads_controls.cpp`

What a GUI control is and its state: class predicates, id, flags, list box row count, toggle state. Every read is guarded: a non-null control can be freed (a panel teardown leaves stale pointers in monitor state for a tick), and a stale one must read as "not this class", not crash.

## Declarations (in source order)

- L17 `uintptr_t VtableOf(void* control)`: The control's vtable address, 0 on fault.
- L25 `int ReadControlId(void* control)`
- L30 `bool IsToggle(void* control)`
- L34 `bool IsSlider(void* control)`
- L38 `bool IsListBox(void* control)`
- L44 `bool IsEditbox(void* control)`: CSWGuiSaveGameEditBox (the save-name popup) is a subclass with its own vtable and the same layout.
- L50 `uint32_t ReadControlBitFlags(void* control)`
- L56 `bool IsControlVisible(void* control)`
- L60 `int ReadListBoxRowCount(void* listBox)`
- L73 `bool ReadListBoxRowArray(void* listBox, void** outData)`: The controls array's data pointer; false when unreadable or null.
- L81 `void* ReadListBoxRow(void* listBox, int index)`
- L92 `int ReadListBoxSelection(void* listBox)`
- L98 `void* ReadListBoxSelectedRow(void* listBox)`
- L102 `int FindListBoxRow(void* listBox, void* row)`
- L117 `bool ReadToggleState(void* toggle)`
- L123 `void DumpControlVtable(void* control, char* out, size_t outSize)`
