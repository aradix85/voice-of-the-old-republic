# menus_extract_internal.h (66 lines)

Source: `patches/Accessibility/menus_extract_internal.h`

Internal to the menus_extract module (menus_extract*.cpp); no other file includes it. The public interface is menus_extract.h. Here: what the files share.

## Declarations (in source order)

- L32 `bool EmitText(char* outBuf, size_t bufSize, const char* text)`: menus_extract.cpp
- L33 `void* ResolveOwnerPanel(void* control, void* ownerPanel)`
- L34 `bool IsPanelDirectChild(void* panel, void* control)`
- L37 `const char* TryButton(void* control, char* outBuf, size_t bufSize)`: menus_extract_controls.cpp
- L38 `const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize)`
- L39 `const char* TryLabel(void* control, char* outBuf, size_t bufSize)`
- L40 `const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize)`
- L41 `const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize)`
- L42 `const char* TryEditbox(void* control, char* outBuf, size_t bufSize)`
- L43 `const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize)`
- L44 `const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize)`
- L45 `const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize)`
- L48 `const char* TryVirtualRowAnchor(void* control, void* owner, char* outBuf, size_t bufSize)`: menus_extract_rows.cpp
- L51 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: menus_extract_icons.cpp
- L52 `const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize)`
- L53 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`
- L54 `const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize)`
- L55 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`
- L58 `const char* TryWorkbenchSlot(void* control, void* owner, char* outBuf, size_t bufSize)`: menus_extract_workbench.cpp
- L59 `const char* TryWorkbenchCreateItemsButton(void* control, void* owner, char* outBuf, size_t bufSize)`
- L62 `const char* TryClassSelectionIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: menus_extract_chargen.cpp
- L63 `const char* TryPortraitCharGenArrow(void* control, void* owner, char* outBuf, size_t bufSize)`

Also 8 constants or variables at namespace scope.
