# core_tick.cpp (553 lines)

Source: `patches/Accessibility/core_tick.cpp`

Per-tick dispatcher — see core_tick.h.

## Declarations (in source order)

- L107 `inline LARGE_INTEGER QpcNow()`
- L118 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC → ms, wrapping at 2^32 exactly like GetTickCount so every existing `now - then >= window` comparison stays valid unsigned arithmetic.
- L125 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L131 `inline void PhaseMark(const char* name, double ms)`
- L136 `LARGE_INTEGER WatchdogBeginTick()`: Reset accumulators + check the inter-tick gap.
- L159 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`: Log the total if our Dispatch overran, naming the slowest phase.
- L173 `DWORD NowMs()`
- L180 `void Dispatch()`
- L550 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour — @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
