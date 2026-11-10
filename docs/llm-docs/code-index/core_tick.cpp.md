# core_tick.cpp (932 lines)

Source: `patches/Accessibility/core_tick.cpp`

Per-tick dispatcher — see core_tick.h.

## Declarations (in source order)

- L111 `inline LARGE_INTEGER QpcNow()`
- L122 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC → ms, wrapping at 2^32 exactly like GetTickCount so every existing `now - then >= window` comparison stays valid unsigned arithmetic.
- L129 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L135 `inline void PhaseMark(const char* name, double ms)`
- L149 `LARGE_INTEGER WatchdogBeginTick()`: Reset accumulators + check the inter-tick gap.
- L172 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`: Log the total if our Dispatch overran, naming the slowest phase.
- L201 `int HeldEngineBoundKey()`: Held-key probe — shared by both reacquire paths below Engine-bound keys a player actually presses when nothing is responding.
- L252 `void RetryColdStartReacquire()`
- L405 `void SampleWorldResponse(ULONGLONG now)`
- L442 `void WatchDeadKeyboard()`
- L552 `DWORD NowMs()`
- L559 `void Dispatch()`
- L929 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour — @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
