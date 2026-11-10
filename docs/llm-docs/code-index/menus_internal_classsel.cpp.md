# menus_internal_classsel.cpp (144 lines)

Source: `patches/Accessibility/menus_internal_classsel.cpp`

The chargen class selection: recognising a class icon, and the class name and description cache that makes each icon speakable.

## Declarations (in source order)

- L49 `bool IsClassSelectionIcon(void* panel, void* control)`: The 6 class icons are an embedded CSWGuiClassSelChar array (kClassSelectionsArrayOffset, stride kClassSelCharSize), each starting with its CSWGuiButton, so an icon control lands exactly on an element start.
- L68 `void ClassDescBaselineCapture(void* panel, const char* text)`
- L78 `bool ClassDescIsEngineWritten(void* panel, const char* text)`
- L87 `const char* ClassLabelCacheLookup(void* panel, void* icon)`
- L99 `void ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: The extractor's cache fill and the outgoing-icon prefill both compose here, so the cached string is identical whichever path wins the race.
- L120 `void ClassLabelCacheStore(void* panel, void* icon, const char* text)`
