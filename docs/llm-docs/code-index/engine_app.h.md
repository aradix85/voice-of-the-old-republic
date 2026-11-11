# engine_app.h (126 lines)

Source: `patches/Accessibility/engine_app.h`

AppManager resolve primitives — the root every engine object walk starts at.

Everything the engine owns hangs off one .data global, the CAppManager singleton, which carries two facades:

*kAddrAppManagerPtr → CAppManager → +0x4 CClientExoApp  (UI / client side) → +0x8 CServerExoApp  (world / AI truth)

Each facade is an 8-byte public shell (vtable@0, internal@4) whose *Internal carries the real state — see project_client_server_architecture for which side answers which question.

Why this file exists -------------------- Before Phase 3 this walk was hand-rolled at every site that needed it: SIX different names for the same +0x8 hop, three private copies of the AppManager pointer, and per-site SEH guards that some callers remembered and others did not. engine_subscreen.cpp said the quiet part out loud — "we duplicate the constant locally ... because the latter are file-local". A missing guard on one of these walks is precisely the F2 crash class, so the guard belongs in one place rather than in every caller's good intentions.

Every primitive here is SEH-guarded and yields nullptr on a null link or a fault, so a caller only ever has to test for nullptr. Callers that go on to CALL an engine function through the returned pointer still need their own __try around that call — the guard here covers the walk, not what you do with the result.

K2 port note: the three constants and the functions below are the entire seam. On a KOTOR 2 executable the global's address changes (and possibly the hop offsets); nothing else in the codebase has to know.

## Declarations (in source order)

- L98 `void* GetAppManager()`: *kAddrAppManagerPtr.
- L102 `void* GetClientApp()`: CClientExoApp facade (AppManager +0x4).
- L106 `void* GetClientAppInternal()`: CClientExoAppInternal (facade +0x4) — the real client state: player_control @+0x2a0, CGuiInGame, the options block, input_class @+0x9c.
- L110 `void* GetClientModule()`: CSWCModule (client internal +0x18) — the client's view of the loaded module.
- L114 `void* GetCamera()`: The active camera (CSWCModule +0x40).
- L118 `void* GetServerApp()`: CServerExoApp facade (AppManager +0x8).
- L123 `void* GetServerAppInternal()`: CServerExoAppInternal (facade +0x4).

Also 8 constants or variables at namespace scope.
