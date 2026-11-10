# engine_radial.cpp (396 lines)

Source: `patches/Accessibility/engine_radial.cpp`

kButtonTextOffset, kLabelGuiStringPtrOffset, kLabelTextOffset, kAurGuiStringCStrOffset, kVtableCAurGUIStringInternal, CExoString

## Declarations (in source order)

- L51 `void* RowActionAddr(void* tam, int row)`
- L57 `void* RowActionButtonAddr(void* tam, int row)`
- L68 `void* FindSelectedActionDescriptor(void* tam, int row)`: Find the CSWGuiInterfaceAction* in action_lists[row].data that matches the engine's currently-selected action_id (field1[target_type*3 + row]).
- L106 `bool ReadInt32(void* base, size_t offset, int32_t* out)`
- L121 `bool ReadGuiStringLocal(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`: Local copy of the gui_string read path used by ExtractAnnounceableText — pulled in here so the engine layer doesn't depend on menus.cpp's helpers.
- L150 `bool ReadCExoStringLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: Inline CExoString fallback.
- L172 `void* ResolveTargetActionMenu()`
- L179 `int RowActionCount(void* tam, int row)`
- L189 `uint32_t ReadSelectedRowActionId(void* tam, int row)`
- L202 `bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize)`
- L235 `bool ReadTargetName(void* tam, char* outBuf, size_t bufSize)`
- L248 `bool DispatchRowAction(void* tam, int row)`
- L260 `bool SelectActionInRow(void* tam, int row, int index)`
- L290 `uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index)`
- L307 `int FindRowIndexByActionId(void* tam, int row, uint32_t actionId)`
- L328 `int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle)`
- L360 `void* CallVtableAsClass(void* gameObject, size_t vtableOffset)`: Call vtable->AsSWCxxx(gameObject) at the given vtable byte-offset.
- L377 `bool IsCreatureClientTarget(uint32_t handle)`
