# focus_guard.cpp (306 lines)

Source: `patches/Accessibility/focus_guard.cpp`

The focus probe: a poll thread that subclasses every game window to log its focus messages, feeds the input release/reacquire on focus loss and regain, swallows the Alt menu mode, and runs the movie-end guard and the Big Picture warning every 100 ms.

## Declarations (in source order)

- L41 `const SubclassedWindow* FindSubclassed(HWND hwnd)`
- L51 `void LogForegroundThief(DWORD activatingThread)`: Which application took the foreground on a focus loss (Steam, Discord, Game Bar overlays, an alt-tab target).
- L77 `const char* ActivateName(WORD w)`
- L86 `LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)`
- L145 `bool IsGameWindowClass(const char* cls)`: The game windows worth subclassing: not the OS input windows, and never the Bink movie window, whose player aborts its queue (closing the game) on a cross-thread wndproc swap mid-playback.
- L152 `BOOL CALLBACK EnumDiagProc(HWND hwnd, LPARAM lp)`
- L174 `void LogAllProcessWindows()`: The baseline window inventory when the poll thread starts; afterwards only newly subclassed windows are logged.
- L184 `BOOL CALLBACK ScanProc(HWND hwnd, LPARAM lp)`: One window of the poll thread's scan (the table's only writer).
- L248 `DWORD WINAPI PollProc(LPVOID)`
- L271 `const char* ProcessExeName(DWORD pid, char* buf, DWORD bufSize)`
- L282 `bool GameOwnsForeground()`
- L290 `void StartFocusProbe()`
