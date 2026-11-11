# core_tick.cpp (371 lines)

Source: `patches/Accessibility/core_tick.cpp`

The per-tick dispatcher (see core_tick.h) and its watchdog.

## Declarations (in source order)

- L97 `inline LARGE_INTEGER QpcNow()`
- L107 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC to ms, wrapping at 2^32 exactly like GetTickCount so every `now - then >= window` comparison stays valid unsigned arithmetic.
- L114 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L120 `inline void PhaseMark(const char* name, double ms)`
- L126 `LARGE_INTEGER WatchdogBeginTick()`: Reset the accumulators and check the gap since the last tick.
- L148 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`
- L162 `DWORD NowMs()`
- L169 `void Dispatch()`
- L368 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour: @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
