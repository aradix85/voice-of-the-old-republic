# trap_watch.cpp (291 lines)

Source: `patches/Accessibility/trap_watch.cpp`

Trap detection watcher — see trap_watch.h for the design and docs/llm-docs/mine-trap-model.md for the engine model.

## Declarations (in source order)

- L72 `TrackedTrap* FindOrAddTracked(uint32_t handle)`
- L83 `void PushFresh(const char* name, const Vector& pos, DWORD now)`
- L96 `void ExpireFresh(DWORD now)`
- L103 `bool TriggerIsTrap(void* trigger)`: SEH-guarded is-mine read (CSWSTrigger.is_trap).
- L116 `void TrapSpokenName(void* obj, bool isMine, char* out, size_t size)`: Localized object name with per-kind fallback (mines placed by scripts occasionally carry no authored name — GetObjectName would fall back to the raw tag, which is modder machinery, not speech).
- L127 `std::string WithClock(const char* label, const Vector& target)`: " , X o'clock, Y metres" via the shared announce format; falls back to the bare label when the viewpoint is unreadable.
- L151 `void Tick()`
- L162 `void ScanNow()`: Forced rescan for the message-enrichment race: the engine appends the mine-detect feedback line in the same tick it updates the detected-by list, which can beat the next 250ms scan.
- L168 `void ScanInternal(DWORD now)`
- L269 `bool PeekFreshMine(char* nameOut, size_t nameSize, Vector& posOut)`
- L282 `void ConsumeFreshMine()`
