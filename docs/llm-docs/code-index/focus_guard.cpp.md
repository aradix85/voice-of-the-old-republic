# focus_guard.cpp (793 lines)

Source: `patches/Accessibility/focus_guard.cpp`

(From focus_guard.h.) Diagnostics for the "menu loaded but unresponsive" / "intro movie plays twice" / "alt-tab needed to wake the engine" class of reports.

LogComApartment reports the COM apartment model the calling thread is committed to. Called once after EnsurePrismInitialized so we can see whether the SAPI backend inside prism.dll has CoInitialize'd the engine's main thread into MTA (which can conflict with Bink / other engine COM paths that expect STA, producing exactly the "fine until the first focus loss, then dead" symptom).

StartFocusProbe spins up a background polling thread that periodically re-enumerates the process's top-level windows and subclasses every game window it hasn't seen before. Subclassed WindowProc logs every WM_ACTIVATE / WM_ACTIVATEAPP / WM_SETFOCUS / WM_KILLFOCUS with the source HWND + class name tag. Polls every 100ms so transient windows (SWMovieWindow recreated between intros) get caught.

Idempotent — repeat calls are no-ops. Safe to call from OnRulesInit (early, before windows exist) and from MainMenu first-sight (late, belt-and-braces) without worrying about double-subclassing.

## Declarations (in source order)

- L73 `const SubclassedWindow* FindSubclassed(HWND hwnd)`
- L81 `const char* ApartmentTypeName(APTTYPE t)`
- L91 `const char* ApartmentQualifierName(APTTYPEQUALIFIER q)`
- L112 `void LogForegroundThief(DWORD activatingThread)`: On a focus-loss (WM_ACTIVATEAPP active=0) the foreground has moved to a window outside our process.
- L151 `const char* ActivateName(WORD w)`
- L160 `LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)`
- L260 `bool IsGameWindowClass(const char* cls)`: True for game-owned windows we want focus events from.
- L283 `void LogOneWindow(HWND hwnd, const char* origin)`
- L306 `BOOL CALLBACK EnumDiagProc(HWND hwnd, LPARAM lp)`
- L317 `void LogAllProcessWindows(const char* origin)`
- L333 `BOOL CALLBACK ScanProc(HWND hwnd, LPARAM lp)`
- L436 `void ForceForeground(HWND target, HWND fg)`: Pull `target` to the foreground from a thread that doesn't own it.
- L456 `void MaybeReclaimForeground()`: Called once per poll iteration.
- L528 `void MaybeReclaimAfterMovie()`
- L592 `bool ContainsNoCase(const char* hay, const char* needle)`: Case-insensitive substring test (ASCII).
- L608 `bool AnyInteractionKeyDown()`: True while any "the user is trying to interact" key is physically down: arrows / Enter / Space / Esc / Tab / Backspace / digits / letters / F-keys.
- L632 `bool ForegroundIsBigPicture()`: Is the current foreground window Steam Big Picture?
- L660 `void MaybeFlagInputBlockedWarning()`: Poll-thread step: if the user pressed an interaction key while Steam Big Picture holds the foreground, queue a spoken warning for the main tick to drain.
- L679 `DWORD WINAPI PollProc(LPVOID)`
- L731 `void LogComApartment(const char* tag)`
- L747 `bool GameOwnsForeground()`
- L755 `void StartFocusProbe()`
- L774 `void DrainInputBlockedWarning()`
- L782 `void ArmStartupForegroundGuard()`
