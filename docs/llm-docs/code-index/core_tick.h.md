# core_tick.h (42 lines)

Source: `patches/Accessibility/core_tick.h`

Mod-wide per-tick dispatcher.

Owns the OnUpdate hook (CSWGuiManager::Update @ 0x0040ce76). Fans out to each subsystem in a fixed order — explicit calls in Dispatch() so the file reads as the canonical "what fires per tick" list.

## Declarations (in source order)

- L13 `void Dispatch()`
- L39 `DWORD NowMs()`: Timestamp of the tick currently being dispatched, in milliseconds, wrapping at 2^32 exactly like GetTickCount — so it drops into the existing `now - then >= window` idiom unchanged.
