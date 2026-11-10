# transitions_preload.cpp (202 lines)

Source: `patches/Accessibility/transitions_preload.cpp`

The destination before a module load: the SetMoveToModuleString hooks (KOTOR 1 and KOTOR 2), the pre-load announcement and the module-load latch that keeps speech quiet through the load.

## Declarations (in source order)

- L24 `bool LooksLikeResref(const char* s)`: A destination module resref is short printable ASCII — "endar_spire", "tar_m02ac", "101PER".
- L41 `bool TryReadExoString(void* base, char* out, size_t outSize)`: `base` is a CExoString* { char* c_string; uint32 length }.
- L52 `bool IsModuleLoadPending()`
- L56 `void NotifyExternalLoadStarting(const char* reason)`
- L67 `void AnnouncePreLoadDestination(void* exoStringPtr)`
- L124 `void LatchPreLoadDestinationK2(void* exoStringPtr)`
- L161 `extern "C" void __cdecl OnSetMoveToModuleString(void* /*serverApp*/, void* arg_addr)`: CServerExoApp::SetMoveToModuleString — entry hook @0x004aecd0.
- L187 `void __cdecl OnSetMoveToModuleStringK2(void* ebp)`
