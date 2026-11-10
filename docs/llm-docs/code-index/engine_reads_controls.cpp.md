# engine_reads_controls.cpp (86 lines)

Source: `patches/Accessibility/engine_reads_controls.cpp`

What a GUI control is and its state: class predicates, id, flags, list box row count, toggle state. Every read is guarded: a non-null control can be freed (a panel teardown leaves stale pointers in monitor state for a tick), and a stale one must read as "not this class", not crash.

## Declarations (in source order)

- L15 `uintptr_t VtableOf(void* control)`: The control's vtable address, 0 on fault.
- L23 `int ReadControlId(void* control)`
- L28 `bool IsToggle(void* control)`
- L32 `bool IsSlider(void* control)`
- L36 `bool IsListBox(void* control)`
- L42 `bool IsEditbox(void* control)`: CSWGuiSaveGameEditBox (the save-name popup) is a subclass with its own vtable and the same layout.
- L48 `uint32_t ReadControlBitFlags(void* control)`
- L54 `bool IsControlVisible(void* control)`
- L58 `int ReadListBoxRowCount(void* listBox)`
- L68 `bool ReadToggleState(void* toggle)`
- L74 `void DumpControlVtable(void* control, char* out, size_t outSize)`
