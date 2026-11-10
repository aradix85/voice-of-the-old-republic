# core_tick.cpp (989 lines)

Source: `patches/Accessibility/core_tick.cpp`

Per-tick dispatcher — see core_tick.h.

## Declarations (in source order)

- L116 `inline LARGE_INTEGER QpcNow()`
- L127 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC → ms, wrapping at 2^32 exactly like GetTickCount so every existing `now - then >= window` comparison stays valid unsigned arithmetic.
- L134 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L140 `inline void PhaseMark(const char* name, double ms)`
- L155 `LARGE_INTEGER WatchdogBeginTick()`: Reset accumulators + check the inter-tick gap.
- L179 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`: Log the total if our Dispatch overran, naming the slowest phase.
- L208 `int HeldEngineBoundKey()`: Held-key probe — shared by both reacquire paths below Engine-bound keys a player actually presses when nothing is responding.
- L259 `void RetryColdStartReacquire()`
- L412 `void SampleWorldResponse(ULONGLONG now)`
- L449 `void WatchDeadKeyboard()`
- L559 `DWORD NowMs()`
- L566 `void Dispatch()`
- L986 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour — @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
