# engine_panels_reads.cpp (117 lines)

Source: `patches/Accessibility/engine_panels_reads.cpp`

Engine reads beside the panel registry: the CGuiInGame and main-interface pointers, the dialog replies, and the workbench upgrade-slot table.

## Declarations (in source order)

- L20 `bool LookupUpgradeSlotType(int slotCustomValue, int category, int* outUpgradeType, uint32_t* outStrRef)`: See the header for why the index lives here rather than at the call sites.
- L55 `void* ResolveGuiInGame()`
- L68 `void* ResolveMainInterface()`
- L80 `bool ReadDialogReplyText(int replyIndex, char* outBuf, size_t bufSize)`
- L105 `int ReadDialogReplyCount()`
