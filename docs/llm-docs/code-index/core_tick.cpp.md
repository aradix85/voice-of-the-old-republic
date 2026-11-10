# core_tick.cpp (935 lines)

Source: `patches/Accessibility/core_tick.cpp`

Per-tick dispatcher — see core_tick.h.

## Declarations (in source order)

- L112 `inline LARGE_INTEGER QpcNow()`
- L123 `inline DWORD QpcToMs(const LARGE_INTEGER& t)`: QPC → ms, wrapping at 2^32 exactly like GetTickCount so every existing `now - then >= window` comparison stays valid unsigned arithmetic.
- L130 `inline double QpcMs(const LARGE_INTEGER& a, const LARGE_INTEGER& b)`
- L136 `inline void PhaseMark(const char* name, double ms)`
- L151 `LARGE_INTEGER WatchdogBeginTick()`: Reset accumulators + check the inter-tick gap.
- L175 `void WatchdogEndTick(const LARGE_INTEGER& tickStart)`: Log the total if our Dispatch overran, naming the slowest phase.
- L204 `int HeldEngineBoundKey()`: Held-key probe — shared by both reacquire paths below Engine-bound keys a player actually presses when nothing is responding.
- L255 `void RetryColdStartReacquire()`
- L408 `void SampleWorldResponse(ULONGLONG now)`
- L445 `void WatchDeadKeyboard()`
- L555 `DWORD NowMs()`
- L562 `void Dispatch()`
- L932 `extern "C" void __cdecl OnUpdate(void* /*unused*/)`: CSWGuiManager::Update detour — @0x40ce76 on KOTOR 1 (argument = the manager, from EBP), @0x4113a9 on KOTOR 2 (argument = the frame pointer).
