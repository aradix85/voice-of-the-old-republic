# engine_picker.cpp (610 lines)

Source: `patches/Accessibility/engine_picker.cpp`

the direct ActionInitiateDialog call.

## Declarations (in source order)

- L120 `bool CallPopulateMenus(void* mainIf)`: One guarded call, correct shape per game.
- L137 `void WriteUInt32(void* base, size_t offset, uint32_t value)`
- L149 `uint32_t ReadUInt32(void* base, size_t offset)`
- L162 `bool CopyCStringSafe(const char* src, char* dst, size_t cap)`: Copy a null-terminated string from src into dst (capacity bytes including terminator) under SEH.
- L184 `void ReadResRef(void* base, size_t offset, char* out, size_t outCap)`: Read an inline CResRef (a 16-byte fixed-buffer string used for engine resource identifiers like "i_dialog").
- L207 `void ReadExoString(void* base, size_t offset, char* out, size_t outCap)`: Read a CExoString (c_string + length pair).
- L223 `void SnapshotDescriptor(void* internal, acc::picker::ActionSnapshot* snap)`: Snapshot the descriptor at internal+0x4c8.
- L248 `bool Drive(uint32_t targetServerHandle, ActionSnapshot* outSnapshot, bool forceRadial, bool populateOnly)`
- L518 `bool ReanchorRadial(uint32_t targetServerHandle)`
- L558 `bool ReadCurrent(ActionSnapshot* outSnapshot)`
- L578 `bool InitiateDialog(uint32_t targetServerHandle)`
