# engine_picker.cpp (349 lines)

Source: `patches/Accessibility/engine_picker.cpp`

The engine's own default action for a target: build its action descriptor the way a mouse hover does, read it, and dispatch it the way a click does, or open the radial menu when there is none (engine_picker.h).

## Declarations (in source order)

- L76 `bool CallPopulateMenus(void* mainIf)`
- L91 `bool BuildDescriptor(void* guiIn, void* internal, uint32_t targetClient)`: Points the main interface at the target and has the engine build its action descriptor for it.
- L102 `bool CallHandleMouseClickInWorld(void* internal)`
- L111 `void WriteUInt32(void* base, size_t offset, uint32_t value)`
- L120 `void CopyCString(const char* src, size_t limit, char* out, size_t outCap)`: Copies a NUL-terminated string of at most `limit` bytes, truncated to the buffer.
- L135 `uint32_t ToClientHandle(uint32_t handle)`: The engine's id for a handle from either namespace: server ids have the high bit clear, the cursor system uses the OR'd client form.
- L141 `void SnapshotDescriptor(void* internal, ActionSnapshot* snap)`: The descriptor the engine last built.
- L164 `void RejectUnusable(ActionSnapshot& snap, uint32_t targetClient)`: Drops a descriptor that is not a usable action on this target.
- L199 `bool OpenRadial(void* mainIf, uint32_t targetClient, int descriptorCount, bool forceRadial)`: No default action, or the radial was asked for: open the radial menu the way the first mouse click on a target does, through the PopulateMenus wrapper.
- L223 `bool DispatchClick(void* internal, uint32_t targetClient, const ActionSnapshot& snap)`: Dispatches the descriptor the way a mouse click does.
- L254 `bool Drive(uint32_t targetServerHandle, ActionSnapshot* outSnapshot, bool forceRadial, bool populateOnly)`
- L296 `bool ReanchorRadial(uint32_t targetServerHandle)`: Drive's radial branch without the diagnostics: it runs on every radial keypress.
- L310 `bool ReadCurrent(ActionSnapshot* outSnapshot)`
- L326 `bool InitiateDialog(uint32_t targetServerHandle)`
