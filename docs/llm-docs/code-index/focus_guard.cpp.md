# focus_guard.cpp (456 lines)

Source: `patches/Accessibility/focus_guard.cpp`

(From focus_guard.h.) Diagnostics for the "menu loaded but unresponsive" / "intro movie plays twice" / "alt-tab needed to wake the engine" class of reports.

StartFocusProbe spins up a background polling thread that periodically re-enumerates the process's top-level windows and subclasses every game window it hasn't seen before. Subclassed WindowProc logs every WM_ACTIVATE / WM_ACTIVATEAPP / WM_SETFOCUS / WM_KILLFOCUS with the source HWND + class name tag. Polls every 100ms so transient windows (SWMovieWindow recreated between intros) get caught.

Idempotent — repeat calls are no-ops. Safe to call from OnRulesInit (early, before windows exist) and from MainMenu first-sight (late, belt-and-braces) without worrying about double-subclassing.

## Declarations (in source order)

- L49 `const SubclassedWindow* FindSubclassed(HWND hwnd)`
- L65 `void LogForegroundThief(DWORD activatingThread)`: On a focus-loss (WM_ACTIVATEAPP active=0) the foreground has moved to a window outside our process.
- L104 `const char* ActivateName(WORD w)`
- L113 `LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)`
- L213 `bool IsGameWindowClass(const char* cls)`: True for game-owned windows we want focus events from.
- L228 `void LogOneWindow(HWND hwnd, const char* origin)`
- L251 `BOOL CALLBACK EnumDiagProc(HWND hwnd, LPARAM lp)`
- L262 `void LogAllProcessWindows(const char* origin)`
- L278 `BOOL CALLBACK ScanProc(HWND hwnd, LPARAM lp)`
- L377 `DWORD WINAPI PollProc(LPVOID)`
- L428 `bool GameOwnsForeground()`
- L436 `void StartFocusProbe()`
