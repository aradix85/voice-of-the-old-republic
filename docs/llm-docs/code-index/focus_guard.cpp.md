# focus_guard.cpp (670 lines)

Source: `patches/Accessibility/focus_guard.cpp`

(From focus_guard.h.) Diagnostics for the "menu loaded but unresponsive" / "intro movie plays twice" / "alt-tab needed to wake the engine" class of reports.

StartFocusProbe spins up a background polling thread that periodically re-enumerates the process's top-level windows and subclasses every game window it hasn't seen before. Subclassed WindowProc logs every WM_ACTIVATE / WM_ACTIVATEAPP / WM_SETFOCUS / WM_KILLFOCUS with the source HWND + class name tag. Polls every 100ms so transient windows (SWMovieWindow recreated between intros) get caught.

Idempotent — repeat calls are no-ops. Safe to call from OnRulesInit (early, before windows exist) and from MainMenu first-sight (late, belt-and-braces) without worrying about double-subclassing.

## Declarations (in source order)

- L50 `const SubclassedWindow* FindSubclassed(HWND hwnd)`
- L66 `void LogForegroundThief(DWORD activatingThread)`: On a focus-loss (WM_ACTIVATEAPP active=0) the foreground has moved to a window outside our process.
- L105 `const char* ActivateName(WORD w)`
- L114 `LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)`
- L214 `bool IsGameWindowClass(const char* cls)`: True for game-owned windows we want focus events from.
- L229 `void LogOneWindow(HWND hwnd, const char* origin)`
- L252 `BOOL CALLBACK EnumDiagProc(HWND hwnd, LPARAM lp)`
- L263 `void LogAllProcessWindows(const char* origin)`
- L279 `BOOL CALLBACK ScanProc(HWND hwnd, LPARAM lp)`
- L382 `void ForceForeground(HWND target, HWND fg)`: Pull `target` to the foreground from a thread that doesn't own it.
- L432 `void MaybeReclaimAfterMovie()`
- L496 `bool ContainsNoCase(const char* hay, const char* needle)`: Case-insensitive substring test (ASCII).
- L512 `bool AnyInteractionKeyDown()`: True while any "the user is trying to interact" key is physically down: arrows / Enter / Space / Esc / Tab / Backspace / digits / letters / F-keys.
- L536 `bool ForegroundIsBigPicture()`: Is the current foreground window Steam Big Picture?
- L564 `void MaybeFlagInputBlockedWarning()`: Poll-thread step: if the user pressed an interaction key while Steam Big Picture holds the foreground, queue a spoken warning for the main tick to drain.
- L583 `DWORD WINAPI PollProc(LPVOID)`
- L634 `bool GameOwnsForeground()`
- L642 `void StartFocusProbe()`
- L661 `void DrainInputBlockedWarning()`
