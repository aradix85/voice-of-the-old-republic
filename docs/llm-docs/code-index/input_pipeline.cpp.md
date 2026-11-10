# input_pipeline.cpp (703 lines)

Source: `patches/Accessibility/input_pipeline.cpp`

chosen variant per slot so bare 4..7 fires the same variant the submenu last announced

## Declarations (in source order)

- L81 `unsigned int NextSeq()`
- L85 `void NoteOverlayEscClosed()`
- L89 `bool ConsumeOverlayEscLatch()`
- L96 `void NoteEditboxSubmitClosed()`
- L100 `bool ConsumeEditboxSubmitLatch()`
- L118 `void PrepareBareDispatchForNarratedTarget()`: SetTarget + RePopulate the engine's action lists against the narrated target (kInvalidObjectId when none resolves — PopulateMenus then builds empty target rows instead of stale ones).
- L150 `extern "C" void __cdecl OnProcessInput(void* /*this_ptr*/)`: CClientExoAppInternal::ProcessInput @ 0x006227e0 — frame-boundary seq tick.
- L172 `extern "C" int __cdecl OnClientHandleInputEvent(void* this_ptr, void* p1_addr, void* p2_addr)`: CClientExoAppInternal::HandleInputEvent @ 0x00621210 — upstream client-app route.
- L697 `extern "C" int __cdecl OnClientHandleInputEventK2(void* this_ptr, void* frame_ebp)`: KOTOR 2 frame-unpacking wrapper — CClientExoAppInternal::HandleInputEvent @0x007B12C0, hooked at 0x007B12EE (kotor2.hooks.toml has the full byte story).
