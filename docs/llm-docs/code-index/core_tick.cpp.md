# core_tick.cpp (387 lines)

Source: `patches/Accessibility/core_tick.cpp`

The per-tick dispatcher (see core_tick.h) and its watchdog.

## Declarations (in source order)

- L101 `inline LARGE_INTEGER QpcNow()`
- L111 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC to ms, wrapping at 2^32 exactly like GetTickCount so every `now - then >= window` comparison stays valid unsigned arithmetic.
- L118 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L124 `inline void PhaseMark(const char* name, double ms)`
- L130 `LARGE_INTEGER WatchdogBeginTick()`: Reset the accumulators and check the gap since the last tick.
- L152 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`
- L166 `DWORD NowMs()`
- L173 `void Dispatch()`
- L384 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour: @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
