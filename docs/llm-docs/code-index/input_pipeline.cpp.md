# input_pipeline.cpp (731 lines)

Source: `patches/Accessibility/input_pipeline.cpp`

chosen variant per slot so bare 4..7 fires the same variant the submenu last announced

## Declarations (in source order)

- L82 `unsigned int NextSeq()`
- L86 `void NoteOverlayEscClosed()`
- L90 `bool ConsumeOverlayEscLatch()`
- L97 `void NoteEditboxSubmitClosed()`
- L101 `bool ConsumeEditboxSubmitLatch()`
- L119 `void PrepareBareDispatchForNarratedTarget()`: SetTarget + RePopulate the engine's action lists against the narrated target (kInvalidObjectId when none resolves — PopulateMenus then builds empty target rows instead of stale ones).
- L151 `extern "C" void __cdecl OnProcessInput(void* /*this_ptr*/)`: CClientExoAppInternal::ProcessInput @ 0x006227e0 — frame-boundary seq tick.
- L173 `extern "C" int __cdecl OnClientHandleInputEvent(void* this_ptr, void* p1_addr, void* p2_addr)`: CClientExoAppInternal::HandleInputEvent @ 0x00621210 — upstream client-app route.
- L725 `extern "C" int __cdecl OnClientHandleInputEventK2(void* this_ptr, void* frame_ebp)`: KOTOR 2 frame-unpacking wrapper — CClientExoAppInternal::HandleInputEvent @0x007B12C0, hooked at 0x007B12EE (kotor2.hooks.toml has the full byte story).
