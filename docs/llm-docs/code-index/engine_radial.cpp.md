# engine_radial.cpp (895 lines)

Source: `patches/Accessibility/engine_radial.cpp`

kButtonTextOffset, kLabelGuiStringPtrOffset, kLabelTextOffset, kAurGuiStringCStrOffset, kVtableCAurGUIStringInternal, CExoString

## Declarations (in source order)

- L196 `bool ReadInt32(void* base, size_t offset, int32_t* out)`
- L211 `bool ReadGuiStringLocal(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`: Local copy of the gui_string read path used by ExtractAnnounceableText — pulled in here so the engine layer doesn't depend on menus.cpp's helpers.
- L240 `bool ReadCExoStringLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: Inline CExoString fallback.
- L262 `void* RowActionAddr(void* tam, int row)`
- L268 `void* RowActionButtonAddr(void* tam, int row)`
- L276 `void* ResolveTargetActionMenu()`
- L283 `int RowActionCount(void* tam, int row)`
- L300 `void* FindSelectedActionDescriptor(void* tam, int row)`: Find the CSWGuiInterfaceAction* in action_lists[row].data that matches the engine's currently-selected action_id (field1[target_type*3 + row]).
- L338 `uint32_t ReadSelectedRowActionId(void* tam, int row)`
- L351 `bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize)`
- L384 `bool ReadTargetName(void* tam, char* outBuf, size_t bufSize)`
- L397 `void LogState(void* tam, const char* tag)`
- L462 `void ReadResRefLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer as NUL-terminated within the 16-byte window.
- L487 `void ReadButtonText(void* button, char* outBuf, size_t bufSize)`: Read a CSWGuiButton's text via every path we know about (gui_string -> inline CExoString) into outBuf.
- L497 `void LogStateWide(void* tam, const char* tag)`
- L579 `bool DispatchRowAction(void* tam, int row)`
- L591 `bool SelectActionInRow(void* tam, int row, int index)`
- L621 `uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index)`
- L638 `int FindRowIndexByActionId(void* tam, int row, uint32_t actionId)`
- L659 `int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle)`
- L693 `void* CallVtableAsClass(void* gameObject, size_t vtableOffset)`: Call vtable->AsSWCxxx(gameObject) at the given vtable byte-offset.
- L712 `void LogTargetDiag(uint32_t targetClient, const char* tag)`
- L876 `bool IsCreatureClientTarget(uint32_t handle)`
