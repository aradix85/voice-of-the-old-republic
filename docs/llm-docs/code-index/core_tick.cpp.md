# core_tick.cpp (1004 lines)

Source: `patches/Accessibility/core_tick.cpp`

Per-tick dispatcher — see core_tick.h.

## Declarations (in source order)

- L119 `inline LARGE_INTEGER QpcNow()`
- L130 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC → ms, wrapping at 2^32 exactly like GetTickCount so every existing `now - then >= window` comparison stays valid unsigned arithmetic.
- L137 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L143 `inline void PhaseMark(const char* name, double ms)`
- L158 `LARGE_INTEGER WatchdogBeginTick()`: Reset accumulators + check the inter-tick gap.
- L182 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`: Log the total if our Dispatch overran, naming the slowest phase.
- L211 `int HeldEngineBoundKey()`: Held-key probe — shared by both reacquire paths below Engine-bound keys a player actually presses when nothing is responding.
- L262 `void RetryColdStartReacquire()`
- L415 `void SampleWorldResponse(ULONGLONG now)`
- L452 `void WatchDeadKeyboard()`
- L562 `DWORD NowMs()`
- L569 `void Dispatch()`
- L1001 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour — @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
