# engine_subscreen.cpp (411 lines)

Source: `patches/Accessibility/engine_subscreen.cpp`

The engine's sub-screen and pause machinery: the overlay pause for the mod's own menus, the unpause safety net when a popup or the last sub-screen closes, the "Paused" / "Resumed" speech, and the hooks on CGuiInGame's Switch/Hide/SetSWGuiStatus and the server's SetPauseState (both games).

## Declarations (in source order)

- L73 `void CallSetSoundMode(void* exoSound, int mode)`
- L83 `bool CallSetPauseState(void* server, bool on, bool own)`: `own` marks the call as the mod's, so that OnSetPauseState, which fires inside it, stays silent.
- L97 `bool ReadUiState(int* modalSize, bool* hasSubScreen)`: The modal-stack size and whether a sub-screen is up; false on a fault.
- L115 `void DispatchUnpauseCleanup(const char* trigger)`: Unpause after a close: server bit 2 off, then the mixer back on (the server round trip does not touch the sound mode).
- L133 `void* PauseClient(const char* what)`: The client app for SetPausedByCombat, or nullptr (logged with `what`).
- L148 `bool CallSetPausedByCombat(void* client, int paused, bool own)`
- L162 `void DispatchOverlayPause(const char* trigger, int paused)`: The overlay's own open and close speech is the cue, as native sub-screens suppress the engine's, so this call counts as our own.
- L172 `bool SetMenuPauseBit(bool on)`
- L177 `void ResumeSoundMixer()`
- L194 `void BeginOverlayPause(OverlayPauseOwner owner)`: The mod's in-world menus have no engine panel, so the engine never pauses for them; these give them a native sub-screen's freeze through the pause key's own path.
- L199 `void EndOverlayPause(OverlayPauseOwner owner)`
- L210 `bool WorldIsPaused()`
- L214 `bool ResumeWorldIfPaused(const char* reason)`
- L242 `void TickInputClassReassert()`: The unpause safety net, on two edges (calling both on one frame is safe): 1.
- L273 `bool ReadSlots(void* p1_addr, void* p2_addr, int* p1, int* p2, uint32_t* callerEip)`: The hooks' parameters are the addresses of the engine's stack slots (the "esp+X" LEA); the return address sits one slot below the first.
- L291 `extern "C" void __cdecl OnSwitchToSWInGameGui(void* thisPtr, int guiId)`: CGuiInGame::SwitchToSWInGameGui, the warm path (a sub-screen is up and another opens; the cold path is ShowSWInGameGui, which the menus monitor covers by polling).
- L306 `extern "C" void __cdecl OnSetSWGuiStatus(void* thisPtr, void* p1_addr, void* p2_addr)`: CGuiInGame::SetSWGuiStatus.
- L326 `extern "C" void __cdecl OnHideSWInGameGui(void* thisPtr, void* p1_addr)`: CGuiInGame::HideSWInGameGui: which engine path closes a sub-screen.
- L340 `extern "C" void __cdecl OnSetPauseState(void* thisPtr, void* p1_addr, void* p2_addr)`: CServerExoAppInternal::SetPauseState (K1 0x004b8110): every pause change.
- L385 `extern "C" void __cdecl OnSwitchToSWInGameGuiK2(void* thisGui, void* ebp)`: CGuiInGame::SwitchToSWInGameGui @0x007CA550, hooked at 0x007CA575.
- L393 `extern "C" void __cdecl OnHideSWInGameGuiK2(void* thisGui, void* ebp)`: CGuiInGame::HideSWInGameGui @0x007CA060, hooked at 0x007CA066.
- L399 `extern "C" void __cdecl OnSetSWGuiStatusK2(void* thisGui, void* ebp)`: CGuiInGame::SetSWGuiStatus @0x007C9C40, hooked at 0x007C9C46.
- L407 `void __cdecl OnSetPauseStateK2(void* thisPtr, void* ebp)`
