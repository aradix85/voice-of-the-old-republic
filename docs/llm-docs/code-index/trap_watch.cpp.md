# trap_watch.cpp (285 lines)

Source: `patches/Accessibility/trap_watch.cpp`

Trap detection watcher — see trap_watch.h for the design and docs/llm-docs/mine-trap-model.md for the engine model.

## Declarations (in source order)

- L73 `TrackedTrap* FindOrAddTracked(uint32_t handle)`
- L84 `void PushFresh(const char* name, const Vector& pos, DWORD now)`
- L97 `void ExpireFresh(DWORD now)`
- L104 `bool TriggerIsTrap(void* trigger)`: SEH-guarded is-mine read (CSWSTrigger.is_trap).
- L111 `void TrapSpokenName(void* obj, bool isMine, char* out, size_t size)`: Localized object name with per-kind fallback (mines placed by scripts occasionally carry no authored name — GetObjectName would fall back to the raw tag, which is modder machinery, not speech).
- L122 `std::string WithClock(const char* label, const Vector& target)`: " , X o'clock, Y metres" via the shared announce format; falls back to the bare label when the viewpoint is unreadable.
- L145 `void Tick()`
- L156 `void ScanNow()`: Forced rescan for the message-enrichment race: the engine appends the mine-detect feedback line in the same tick it updates the detected-by list, which can beat the next 250ms scan.
- L162 `void ScanInternal(DWORD now)`
- L263 `bool PeekFreshMine(char* nameOut, size_t nameSize, Vector& posOut)`
- L276 `void ConsumeFreshMine()`
