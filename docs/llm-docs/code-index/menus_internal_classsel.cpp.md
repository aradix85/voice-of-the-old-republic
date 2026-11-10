# menus_internal_classsel.cpp (153 lines)

Source: `patches/Accessibility/menus_internal_classsel.cpp`

The chargen class selection: recognising a class icon, and the class name and description cache that makes each icon speakable.

## Declarations (in source order)

- L49 `bool IsClassSelectionIcon(void* panel, void* control)`: Positional detector for chargen class-icon buttons.
- L70 `void ClassDescBaselineCapture(void* panel, const char* text)`
- L80 `bool ClassDescIsEngineWritten(void* panel, const char* text)`
- L89 `const char* ClassLabelCacheLookup(void* panel, void* icon)`
- L107 `void ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: Compose what the user hears for one class icon: the class name, followed by the blurb the engine renders beneath it, when there is one.
- L129 `void ClassLabelCacheStore(void* panel, void* icon, const char* text)`
