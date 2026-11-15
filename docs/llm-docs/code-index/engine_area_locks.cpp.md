# engine_area_locks.cpp (207 lines)

Source: `patches/Accessibility/engine_area_locks.cpp`

What it takes to open a locked door or container: a key, the Security DC, and who in the party has the best Security.

## Declarations (in source order)

- L26 `bool ReadLockFieldsRaw(void* obj, size_t lockedOff, size_t keyReqOff, size_t dcOff, bool dcIsShort, LockFields& out)`: One SEH-guarded read of an object's lock fields.
- L46 `bool OpenerHoldsKey(void* keyName)`: Whether the creature under control has the key this lock names, asked the way the game asks when that creature tries the lock (see the addresses in engine_area.h).
- L67 `void LogLockOnce(void* obj, int kind, const LockFields& f, const char* key, LockNeed need, const char* failScript = "")`: One log line per object and answer (recent ones remembered), so the raw values can be checked against the object's UTD / UTP template, and a lock whose answer changes (the key was found) is logged again.
- L93 `void AppendLockNeed(void* obj, char* outBuf, size_t bufSize)`: ", opens only through the story" / ", needs a key" / ", you have the key" / ", Security 15" after the "locked" word, so the player knows whether to bring a key, a slicer, or nothing at all (KOTOR 1; see ReadLockNeed).
- L133 `LockNeed ReadLockNeed(void* gameObject, int& outDc, bool* outLocked)`
