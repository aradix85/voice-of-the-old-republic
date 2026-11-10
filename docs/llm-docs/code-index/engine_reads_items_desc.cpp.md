# engine_reads_items_desc.cpp (234 lines)

Source: `patches/Accessibility/engine_reads_items_desc.cpp`

An item's description, split into its blocks (tags, values, properties, description text).

## Declarations (in source order)

- L18 `void AddProps(uintptr_t builder, void* item, CExoString* acc)`: One of the engine's per-category property builders.
- L25 `bool ReadBaseItemFlags(void* item, uint8_t& itemType, uint8_t& weaponType)`: Read item_type / weapon_type via CSWItem::GetBaseItem (the CSWItem subobject is at offset 0 of CSWSItem, so the item pointer is a valid `this`).
- L57 `void ComputeSectionOffsets(void* item, uint8_t weaponType, size_t& offTags, size_t& offValues, size_t& offProps)`: Append the engine's own per-category builders to ONE accumulator, in the same order and with the same guards GetPropertyDescription uses, recording the visible (strlen) length after each section.
- L110 `bool ReadItemDescriptionViaTlk(void* item, char* outBuf, size_t bufSize, uint32_t& outStrref)`: Resolve the item's identified-description text through the TLK strref, bypassing the (sometimes corrupt) inline CExoLocString substring that GetPropertyDescription/GetString prefer.
- L124 `void CopySlice(char* dst, size_t cap, const char* src, size_t start, size_t end)`: Copy src[start..end) into dst (NUL-terminated, bounded by cap).
- L135 `bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize)`
- L156 `bool BuildItemDescriptionBlocks(void* item, ItemDescriptionBlocks* out)`
