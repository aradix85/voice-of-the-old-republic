# engine_picker.cpp (333 lines)

Source: `patches/Accessibility/engine_picker.cpp`

The engine's own default action for a target: build its action descriptor the way a mouse hover does, read it, and dispatch it the way a click does, or open the radial menu when there is none (engine_picker.h).

## Declarations (in source order)

- L67 `bool CallPopulateMenus(void* mainIf)`
- L82 `bool BuildDescriptor(void* guiIn, void* internal, uint32_t targetClient)`: Points the main interface at the target and has the engine build its action descriptor for it.
- L93 `bool CallHandleMouseClickInWorld(void* internal)`
- L104 `void CopyCString(const char* src, size_t limit, char* out, size_t outCap)`: Copies a NUL-terminated string of at most `limit` bytes, truncated to the buffer.
- L119 `uint32_t ToClientHandle(uint32_t handle)`: The engine's id for a handle from either namespace: server ids have the high bit clear, the cursor system uses the OR'd client form.
- L125 `void SnapshotDescriptor(void* internal, ActionSnapshot* snap)`: The descriptor the engine last built.
- L148 `void RejectUnusable(ActionSnapshot& snap, uint32_t targetClient)`: Drops a descriptor that is not a usable action on this target.
- L183 `bool OpenRadial(void* mainIf, uint32_t targetClient, int descriptorCount, bool forceRadial)`: No default action, or the radial was asked for: open the radial menu the way the first mouse click on a target does, through the PopulateMenus wrapper.
- L207 `bool DispatchClick(void* internal, uint32_t targetClient, const ActionSnapshot& snap)`: Dispatches the descriptor the way a mouse click does.
- L238 `bool Drive(uint32_t targetServerHandle, ActionSnapshot* outSnapshot, bool forceRadial, bool populateOnly)`
- L280 `bool ReanchorRadial(uint32_t targetServerHandle)`: Drive's radial branch without the diagnostics: it runs on every radial keypress.
- L294 `bool ReadCurrent(ActionSnapshot* outSnapshot)`
- L310 `bool InitiateDialog(uint32_t targetServerHandle)`
