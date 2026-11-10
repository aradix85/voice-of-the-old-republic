# engine_radial_internal.h (110 lines)

Source: `patches/Accessibility/engine_radial_internal.h`

Internal to the engine_radial module (engine_radial*.cpp); no other file includes it. The public interface is engine_radial.h. Here: what the files share.

## Declarations (in source order)

- L94 `typedef void (__thiscall* PFN_DoTargetAction)(void* this_, int row, int pad)`: DoTargetAction takes (this, int row) but does ret 8 (Ghidra BYTES_PURGED="8"): it purges a second dword it doesn't use.
- L95 `typedef void* (__thiscall* PFN_GetGameObject)(void* exoApp, uint32_t handle)`
- L96 `typedef bool (__thiscall* PFN_GetCanUseSkill)(void* stats, uint16_t skillIdx)`
- L98 `inline bool ReadInt32(void* base, size_t offset, int32_t* out)`
- L104 `inline void* CallVtableAsClass(void* gameObject, size_t vtableOffset)`: The game object's AsSWCxxx downcast at the given vtable byte offset, or nullptr.

Also 20 constants or variables at namespace scope.
