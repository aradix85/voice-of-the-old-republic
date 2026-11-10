# engine_reads.cpp (151 lines)

Source: `patches/Accessibility/engine_reads.cpp`

The read primitives every engine read goes through: guarded field reads, the downcast call, CExoString and TLK text. The one SEH layer.

## Declarations (in source order)

- L19 `void* CallDowncast(void* control, int vtableIndex)`
- L32 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`
- L59 `uint32_t ReadU32(void* base, size_t offset)`
- L69 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`: SEH-guarded single-field reads for possibly-dead engine pointers.
- L80 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L91 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L118 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: Out-arg: the engine copy-constructs `out` from a stack-local CExoString, allocating a fresh c_string via its own CRT.
- L142 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`
