# menus_extract_internal.h (88 lines)

Source: `patches/Accessibility/menus_extract_internal.h`

Internal to the menus_extract module (menus_extract*.cpp); no other file includes it. The public interface is menus_extract.h. Here: what the files share.

## Declarations (in source order)

- L53 `bool EmitText(char* outBuf, size_t bufSize, const char* text)`: menus_extract.cpp
- L54 `void* ResolveOwnerPanel(void* control, void* ownerPanel)`
- L55 `bool IsPanelDirectChild(void* panel, void* control)`
- L58 `const char* TryTooltip(void* control, char* outBuf, size_t bufSize)`: menus_extract_controls.cpp
- L59 `const char* TryButton(void* control, char* outBuf, size_t bufSize)`
- L60 `const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize)`
- L61 `const char* TryLabel(void* control, char* outBuf, size_t bufSize)`
- L62 `const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize)`
- L63 `const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize)`
- L64 `const char* TryEditbox(void* control, char* outBuf, size_t bufSize)`
- L65 `const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize)`
- L66 `const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize)`
- L67 `const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize)`
- L70 `const char* TryVirtualRowAnchor(void* control, void* ownerPanel, char* outBuf, size_t bufSize)`: menus_extract_rows.cpp
- L73 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: menus_extract_icons.cpp
- L74 `const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize)`
- L75 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`
- L76 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L77 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`
- L80 `const char* TryWorkbenchSlot(void* control, void* owner, char* outBuf, size_t bufSize)`: menus_extract_workbench.cpp
- L81 `const char* TryWorkbenchCreateItemsButton(void* control, void* owner, char* outBuf, size_t bufSize)`
- L84 `const char* TryClassSelectionIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: menus_extract_chargen.cpp
- L85 `const char* TryPortraitCharGenArrow(void* control, void* owner, char* outBuf, size_t bufSize)`

Also 8 constants or variables at namespace scope.
