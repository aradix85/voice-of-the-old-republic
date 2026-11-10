# engine_reads_items_internal.h (27 lines)

Source: `patches/Accessibility/engine_reads_items_internal.h`

Internal to the item reads (engine_reads_items*.cpp); no other file includes it. The public interface is engine_reads.h. Here: what the files share.

## Declarations (in source order)

- L19 `void* ResolveItemFromServerHandle(uint32_t serverHandle)`: engine_reads_items.cpp
- L20 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`
- L24 `bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize)`: engine_reads_items_desc.cpp
