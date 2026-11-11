# engine_radial.cpp (268 lines)

Source: `patches/Accessibility/engine_radial.cpp`

The engine's target action menu (CSWGuiTargetActionMenu): its rows, their action lists and selection, and dispatching a row's action.

## Declarations (in source order)

- L46 `bool ValidRow(int row)`
- L55 `ActionList ReadActionList(unsigned char* tam, int row)`
- L61 `unsigned char* Entry(const ActionList& list, int index)`
- L65 `int32_t EntryId(const ActionList& list, int index)`
- L72 `int32_t* SelectedIdSlot(unsigned char* tam, int row)`: The selected-action slot for this row under the menu's current target type, or nullptr when the type is out of range.
- L80 `void* RowActionButtonAddr(void* tam, int row)`
- L90 `void* FindSelectedActionDescriptor(void* tam, int row)`: The descriptor in action_lists[row] whose id is the engine's selected one.
- L112 `void* ResolveTargetActionMenu()`
- L119 `int RowActionCount(void* tam, int row)`
- L128 `uint32_t ReadSelectedRowActionId(void* tam, int row)`
- L136 `bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize)`
- L158 `bool ReadTargetName(void* tam, char* outBuf, size_t bufSize)`
- L167 `bool DispatchRowAction(void* tam, int row)`
- L179 `bool SelectActionInRow(void* tam, int row, int index)`
- L200 `uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index)`
- L211 `int FindRowIndexByActionId(void* tam, int row, uint32_t actionId)`
- L225 `int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle)`
- L249 `bool IsCreatureClientTarget(uint32_t handle)`
