# engine_reads.cpp (608 lines)

Source: `patches/Accessibility/engine_reads.cpp`

(From engine_reads.h.) SEH-guarded read helpers for KOTOR GUI controls. Safe from hook handlers that may run during engine mid-teardown — every deref is __try-wrapped, every potentially-stale pointer is validated before use.

## Declarations (in source order)

- L27 `int ReadObjectDamageLevel(void* serverObject)`: KOTOR 1: CSWSObject::GetDamageLevel @0x004cb020.
- L65 `int ReadCreatureCurrentHitPoints(void* serverCreature)`
- L76 `int ReadCreatureMaxHitPoints(void* serverCreature)`
- L87 `int ReadObjectCurrentHitPoints(void* serverObject)`
- L98 `bool IsPlotObject(void* serverObject)`
- L109 `bool ReadControlNameFields(void* control, const char*& outTip, uint32_t& outTipLen, int& outId)`
- L136 `void* CallDowncast(void* control, int vtableIndex)`
- L149 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`
- L176 `uint32_t ReadU32(void* base, size_t offset)`
- L186 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`: SEH-guarded single-field reads for possibly-dead engine pointers.
- L197 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L208 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L235 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: Out-arg: the engine copy-constructs `out` from a stack-local CExoString, allocating a fresh c_string via its own CRT.
- L261 `bool EqualsTrimmed(const char* a, const char* b)`: See engine_reads.h for why every engine-rendered-vs-TLK compare has to come through here rather than strcmp.
- L273 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`
- L292 `bool LooksLikeReadableText(const char* buf, size_t len)`: True iff buf looks like real localised text rather than uninitialised memory.
- L313 `bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize)`
- L391 `bool IsGlyphMarkerByte(unsigned char ch)`: True for the byte-level glyph markers KOTOR 2 embeds in a rendered gui_string.
- L395 `bool ReadGuiString(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`
- L445 `bool ExtractTextOrStrRefIndirect(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`
- L478 `bool ReadLabelText(void* label, char* outBuf, size_t bufSize)`
- L498 `bool ReadButtonText(void* button, char* outBuf, size_t bufSize)`
- L518 `bool IsToggle(void* control)`
- L531 `bool IsSlider(void* control)`: Vtable-identity predicates run from per-tick menu monitors.
- L541 `uint32_t ReadControlBitFlags(void* control)`
- L551 `bool IsControlVisible(void* control)`
- L555 `int ReadListBoxRowCount(void* listBox)`
- L568 `bool IsListBox(void* control)`
- L578 `bool IsEditbox(void* control)`
- L592 `bool ReadToggleState(void* toggle)`
- L596 `void DumpControlVtable(void* control, char* out, size_t outSize)`
