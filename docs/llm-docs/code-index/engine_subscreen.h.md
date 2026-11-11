# engine_subscreen.h (117 lines)

Source: `patches/Accessibility/engine_subscreen.h`

CGuiInGame::SwitchToSWInGameGui detour + MessageBoxModal close cleanup.

SwitchToSWInGameGui is the engine's universal "open sub-screen" entry. Without our cleanup, an already-open sub-screen stays alive in panels[] when the user switches to a different one (InGameOptions's OnQuit reorders panels[] without popping). Stale entries accumulate → "menu opens in background" / "Esc fires quit-confirm".

Single-site fix: if a sub-screen is already active when the hook fires, call PrevSWInGameGui first, then let the original push onto clean panels[]. Covers strip-Enter, M/I/J hotkeys, every other invocation.

## Declarations (in source order)

- L34 `void TickInputClassReassert()`: Edge-triggered on modal_stack non-zero → 0.
- L53 `enum class OverlayPauseOwner`: In-world overlay pause hold.
- L60 `void BeginOverlayPause(OverlayPauseOwner owner)`
- L61 `void EndOverlayPause(OverlayPauseOwner owner)`
- L69 `bool WorldIsPaused()`: True when the in-world simulation is currently frozen by any pause source (manual pause key, combat auto-pause, or one of our overlay holds).
- L78 `bool ResumeWorldIfPaused(const char* reason)`: Resume the real world pause (CClientExoApp::SetPausedByCombat(0,4,0)) IF the world is currently paused; no-op (returns false) when already running.
- L83 `bool SetMenuPauseBit(bool on)`: The server's menu pause bit (CServerExoApp::SetPauseState, bit 2), set or cleared.
- L87 `void ResumeSoundMixer()`: The mixer back to playing (SetSoundMode 0): the server round trip of an unpause does not touch the sound mode.
- L93 `extern "" void __cdecl OnSwitchToSWInGameGui(void* thisPtr, int guiId)`: Detour @0x0062cf2d (5-byte cut after EBX = GUI_id is loaded).
- L99 `extern "" void __cdecl OnHideSWInGameGui(void* thisPtr, void* p1_addr)`: Diagnostic detour at HideSWInGameGui @0x0062cba0.
- L106 `extern "" void __cdecl OnSetSWGuiStatus(void* thisPtr, void* p1_addr, void* p2_addr)`: Diagnostic detour at SetSWGuiStatus @0x0062aa00.
- L114 `extern "" void __cdecl OnSwitchToSWInGameGuiK2(void* thisGui, void* ebp)`: KOTOR 2 frame-unpacking wrappers (Batch 2) — the hooks pass ECX (`this`) plus EBP; each wrapper does the frame arithmetic and forwards to the shared handler above.
- L115 `extern "" void __cdecl OnHideSWInGameGuiK2(void* thisGui, void* ebp)`
- L116 `extern "" void __cdecl OnSetSWGuiStatusK2(void* thisGui, void* ebp)`

Also 1 constants or variables at namespace scope.
