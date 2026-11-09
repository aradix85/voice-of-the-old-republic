# engine_radial.cpp (938 lines)

Source: `patches/Accessibility/engine_radial.cpp`

kButtonTextOffset, kLabelGuiStringPtrOffset, kLabelTextOffset, kAurGuiStringCStrOffset, kVtableCAurGUIStringInternal, CExoString

## Declarations (in source order)

- L202 `bool ReadInt32(void* base, size_t offset, int32_t* out)`
- L217 `bool ReadGuiStringLocal(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`: Local copy of the gui_string read path used by ExtractAnnounceableText — pulled in here so the engine layer doesn't depend on menus.cpp's helpers.
- L246 `bool ReadCExoStringLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: Inline CExoString fallback.
- L268 `void* RowActionAddr(void* tam, int row)`
- L274 `void* RowActionButtonAddr(void* tam, int row)`
- L284 `void* ResolveTargetActionMenu()`
- L291 `int RowActionCount(void* tam, int row)`
- L308 `void* FindSelectedActionDescriptor(void* tam, int row)`: Find the CSWGuiInterfaceAction* in action_lists[row].data that matches the engine's currently-selected action_id (field1[target_type*3 + row]).
- L346 `uint32_t ReadSelectedRowActionId(void* tam, int row)`
- L359 `bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize)`
- L392 `bool ReadTargetName(void* tam, char* outBuf, size_t bufSize)`
- L405 `void LogState(void* tam, const char* tag)`
- L470 `void ReadResRefLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer as NUL-terminated within the 16-byte window.
- L495 `void ReadButtonText(void* button, char* outBuf, size_t bufSize)`: Read a CSWGuiButton's text via every path we know about (gui_string -> inline CExoString) into outBuf.
- L505 `void LogStateWide(void* tam, const char* tag)`
- L587 `bool SelectNextActionInRow(void* tam, int row)`
- L599 `bool SelectPrevActionInRow(void* tam, int row)`
- L611 `bool DispatchRowAction(void* tam, int row)`
- L623 `bool SelectActionInRow(void* tam, int row, int index)`
- L653 `uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index)`
- L670 `int FindRowIndexByActionId(void* tam, int row, uint32_t actionId)`
- L691 `int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle)`
- L721 `void* GetRowActionButton(void* tam, int row)`
- L736 `void* CallVtableAsClass(void* gameObject, size_t vtableOffset)`: Call vtable->AsSWCxxx(gameObject) at the given vtable byte-offset.
- L755 `void LogTargetDiag(uint32_t targetClient, const char* tag)`
- L919 `bool IsCreatureClientTarget(uint32_t handle)`
