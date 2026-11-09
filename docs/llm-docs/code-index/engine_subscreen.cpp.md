# engine_subscreen.cpp (698 lines)

Source: `patches/Accessibility/engine_subscreen.cpp`

(From engine_subscreen.h.) CGuiInGame::SwitchToSWInGameGui detour + MessageBoxModal close cleanup.

SwitchToSWInGameGui is the engine's universal "open sub-screen" entry. Without our cleanup, an already-open sub-screen stays alive in panels[] when the user switches to a different one (InGameOptions's OnQuit reorders panels[] without popping). Stale entries accumulate → "menu opens in background" / "Esc fires quit-confirm".

Single-site fix: if a sub-screen is already active when the hook fires, call PrevSWInGameGui first, then let the original push onto clean panels[]. Covers strip-Enter, M/I/J hotkeys, every other invocation.

## Declarations (in source order)

- L70 `void CallSetSoundMode(void* exoSound, int mode)`: One call site for both games' SetSoundMode shapes.
- L108 `void DispatchUnpauseCleanup(const char* trigger)`: Dispatch the two-step unpause + audio-resync.
- L196 `void DispatchOverlayPause(const char* trigger, int paused)`
- L252 `void BeginOverlayPause(OverlayPauseOwner owner)`
- L257 `void EndOverlayPause(OverlayPauseOwner owner)`
- L269 `bool WorldIsPaused()`
- L273 `bool ResumeWorldIfPaused(const char* reason)`
- L314 `void TickInputClassReassert()`
- L392 `extern "C" void __cdecl OnSwitchToSWInGameGui(void* thisPtr, int guiId)`
- L449 `extern "C" void __cdecl OnSetSWGuiStatus(void* thisPtr, void* p1_addr, void* p2_addr)`: Diagnostic: every CGuiInGame::SetSWGuiStatus call logs the new status value plus the calling instruction's return EIP.
- L519 `extern "C" void __cdecl OnHideSWInGameGui(void* thisPtr, void* p1_addr)`: Diagnostic: every CGuiInGame::HideSWInGameGui call logs the caller EIP.
- L553 `extern "C" void __cdecl OnSetPauseState(void* thisPtr, void* p1_addr, void* p2_addr)`: CServerExoAppInternal::SetPauseState @ 0x004b8110 — fires on every pause-state mutation.
- L643 `extern "C" void __cdecl OnSwitchToSWInGameGuiK2(void* thisGui, void* ebp)`: CGuiInGame::SwitchToSWInGameGui @0x007CA550, hooked at 0x007CA575 — after the prologue, before the guards (mirroring KOTOR 1's pre-guard cut).
- L660 `extern "C" void __cdecl OnHideSWInGameGuiK2(void* thisGui, void* ebp)`: CGuiInGame::HideSWInGameGui @0x007CA060, hooked at 0x007CA066.
- L670 `extern "C" void __cdecl OnSetSWGuiStatusK2(void* thisGui, void* ebp)`: CGuiInGame::SetSWGuiStatus @0x007C9C40, hooked at 0x007C9C46.
- L691 `void __cdecl OnSetPauseStateK2(void* thisPtr, void* ebp)`
