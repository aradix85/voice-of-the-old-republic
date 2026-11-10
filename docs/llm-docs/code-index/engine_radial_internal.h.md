# engine_radial_internal.h (113 lines)

Source: `patches/Accessibility/engine_radial_internal.h`

Internal to the engine_radial module (engine_radial*.cpp); no other file includes it. The public interface is engine_radial.h. Here: what the files share.

## Declarations (in source order)

- L99 `typedef void (__thiscall* PFN_DoTargetAction)(void* this_, int row, int pad)`: DoTargetAction takes (this, int row) but does ret 8 (Ghidra BYTES_PURGED="8"): it purges a second dword it doesn't use.
- L100 `typedef void* (__thiscall* PFN_GetGameObject)(void* exoApp, uint32_t handle)`
- L101 `typedef void* (__thiscall* PFN_AsClass)(void* gameObject)`
- L102 `typedef bool (__thiscall* PFN_GetCanUseSkill)(void* stats, uint16_t skillIdx)`
- L107 `bool ReadInt32(void* base, size_t offset, int32_t* out)`: engine_radial.cpp
- L108 `bool ReadGuiStringLocal(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`
- L109 `bool ReadCExoStringLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`
- L110 `void* CallVtableAsClass(void* gameObject, size_t vtableOffset)`

Also 21 constants or variables at namespace scope.
