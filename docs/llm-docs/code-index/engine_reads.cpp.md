# engine_reads.cpp (623 lines)

Source: `patches/Accessibility/engine_reads.cpp`

(From engine_reads.h.) SEH-guarded read helpers for KOTOR GUI controls. Safe from hook handlers that may run during engine mid-teardown — every deref is __try-wrapped, every potentially-stale pointer is validated before use.

## Declarations (in source order)

- L27 `int ReadObjectDamageLevel(void* serverObject)`: KOTOR 1: CSWSObject::GetDamageLevel @0x004cb020.
- L65 `int ReadCreatureCurrentHitPoints(void* serverCreature)`
- L76 `int ReadCreatureMaxHitPoints(void* serverCreature)`
- L87 `int ReadObjectCurrentHitPoints(void* serverObject)`
- L98 `bool IsPlotObject(void* serverObject)`
- L109 `bool ReadControlNameFields(void* control, const char*& outTip, uint32_t& outTipLen, int& outId)`
- L135 `int ReadControlId(void* control)`
- L145 `bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen)`
- L165 `void* CallDowncast(void* control, int vtableIndex)`
- L178 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`
- L205 `uint32_t ReadU32(void* base, size_t offset)`
- L215 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`: SEH-guarded single-field reads for possibly-dead engine pointers.
- L226 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L237 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L264 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: Out-arg: the engine copy-constructs `out` from a stack-local CExoString, allocating a fresh c_string via its own CRT.
- L288 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`
- L307 `bool LooksLikeReadableText(const char* buf, size_t len)`: True iff buf looks like real localised text rather than uninitialised memory.
- L328 `bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize)`
- L406 `bool IsGlyphMarkerByte(unsigned char ch)`: True for the byte-level glyph markers KOTOR 2 embeds in a rendered gui_string.
- L410 `bool ReadGuiString(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`
- L460 `bool ExtractTextOrStrRefIndirect(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`
- L493 `bool ReadLabelText(void* label, char* outBuf, size_t bufSize)`
- L513 `bool ReadButtonText(void* button, char* outBuf, size_t bufSize)`
- L533 `bool IsToggle(void* control)`
- L546 `bool IsSlider(void* control)`: Vtable-identity predicates run from per-tick menu monitors.
- L556 `uint32_t ReadControlBitFlags(void* control)`
- L566 `bool IsControlVisible(void* control)`
- L570 `int ReadListBoxRowCount(void* listBox)`
- L583 `bool IsListBox(void* control)`
- L593 `bool IsEditbox(void* control)`
- L607 `bool ReadToggleState(void* toggle)`
- L611 `void DumpControlVtable(void* control, char* out, size_t outSize)`
