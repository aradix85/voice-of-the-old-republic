# engine_radial.cpp (269 lines)

Source: `patches/Accessibility/engine_radial.cpp`

The engine's target action menu (CSWGuiTargetActionMenu): its rows, their action lists and selection, and dispatching a row's action.

## Declarations (in source order)

- L47 `bool ValidRow(int row)`
- L56 `ActionList ReadActionList(unsigned char* tam, int row)`
- L62 `unsigned char* Entry(const ActionList& list, int index)`
- L66 `int32_t EntryId(const ActionList& list, int index)`
- L73 `int32_t* SelectedIdSlot(unsigned char* tam, int row)`: The selected-action slot for this row under the menu's current target type, or nullptr when the type is out of range.
- L81 `void* RowActionButtonAddr(void* tam, int row)`
- L91 `void* FindSelectedActionDescriptor(void* tam, int row)`: The descriptor in action_lists[row] whose id is the engine's selected one.
- L113 `void* ResolveTargetActionMenu()`
- L120 `int RowActionCount(void* tam, int row)`
- L129 `uint32_t ReadSelectedRowActionId(void* tam, int row)`
- L137 `bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize)`
- L159 `bool ReadTargetName(void* tam, char* outBuf, size_t bufSize)`
- L168 `bool DispatchRowAction(void* tam, int row)`
- L180 `bool SelectActionInRow(void* tam, int row, int index)`
- L201 `uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index)`
- L212 `int FindRowIndexByActionId(void* tam, int row, uint32_t actionId)`
- L226 `int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle)`
- L250 `bool IsCreatureClientTarget(uint32_t handle)`
