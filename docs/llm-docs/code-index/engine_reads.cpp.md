# engine_reads.cpp (150 lines)

Source: `patches/Accessibility/engine_reads.cpp`

The read primitives every engine read goes through: guarded field reads, the downcast call, CExoString and TLK text. The one SEH layer.

## Declarations (in source order)

- L19 `void* CallDowncast(void* control, int vtableIndex)`
- L32 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`
- L51 `uint32_t ReadU32(void* base, size_t offset)`
- L56 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`
- L67 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L78 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L89 `uintptr_t ReadVtable(void* object)`
- L95 `bool HasVtable(void* object, uintptr_t vtable)`
- L99 `bool TryReadFloat(void* base, size_t offset, float* out)`
- L116 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: The engine copy-constructs the result into `tmp`, allocating c_string with its own CRT.
- L140 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`
