# engine_reads.cpp (173 lines)

Source: `patches/Accessibility/engine_reads.cpp`

The read primitives every engine read goes through: guarded field reads, the downcast call, CExoString and TLK text. The one SEH layer.

## Declarations (in source order)

- L19 `void* ReadVirtual(void* object, size_t slotOffset)`
- L23 `PFN_GuiHandleInputEvent ReadHandleInputEvent(void* object)`
- L28 `void* CallDowncast(void* object, int vtableIndex)`
- L39 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`
- L58 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`
- L69 `bool TryReadU8(void* base, size_t offset, uint8_t* out)`
- L79 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L90 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L101 `bool TryReadVector(void* base, size_t offset, Vector* out)`
- L112 `uintptr_t ReadVtable(void* object)`
- L118 `bool HasVtable(void* object, uintptr_t vtable)`
- L122 `bool TryReadFloat(void* base, size_t offset, float* out)`
- L139 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: The engine copy-constructs the result into `tmp`, allocating c_string with its own CRT.
- L163 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`
