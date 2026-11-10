# log.cpp (465 lines)

Source: `patches/Accessibility/log.cpp`

(From log.h.) Logging primitives. The full log always goes to the session file; it also mirrors to OutputDebugStringA, but ONLY when a debugger is attached (an idle DBWin listener otherwise stalls the game's main tick — see log.cpp).

Conventions: every line carries a "Tag" prefix (first arg). Trace/Edge/ Once collapse high-frequency repeats while preserving the suppressed count — full fidelity without flooding. BlockLog does the same for multi-line snapshots (a panel walk, a manager-stack dump): identical consecutive blocks fold to a "(repeated Nx)" summary. No verbosity levels; dedup is structural.

## Declarations (in source order)

- L27 `Locker()`
- L28 `~Locker()`
- L73 `uint64_t FnvAdd(uint64_t h, char c)`
- L77 `uint64_t Fnv1a64(const char* p, size_t n)`
- L86 `Entry* GetOrCreate(Entry (&table)[N], int& count, const char* tag)`
- L97 `void RawWriteLocked(const char* tag, const char* content)`: One line to the file, no dedup.
- L136 `void FlushTraceLocked(DedupEntry* e)`: The three flushes write a pending count, if any, and reset it.
- L144 `void FlushEdgeHoldLocked(DedupEntry* e)`
- L152 `void FlushBlockLocked(BlockEntry* e)`
- L162 `void EmitBlockLinesLocked(const char* tag, const char* buf)`: A '\n'-separated block as one tagged line per segment.
- L179 `void SweepStaleLocked()`: Writes out the counts of tags that have gone quiet.
- L196 `DedupEntry* EntryOrWriteLocked(const char* tag, const char* message)`: The entry for tag.
- L204 `void Init(HINSTANCE hinstDLL)`
- L260 `void Shutdown()`
- L277 `void Write(const char* tag, const char* fmt, ...)`
- L291 `void Trace(const char* tag, const char* fmt, ...)`
- L316 `void Once(const char* tag, const char* fmt, ...)`
- L334 `void Edge(const char* tag, int state, const char* fmt, ...)`
- L359 `BlockLog::BlockLog(const char* tag) : tag_(tag ? tag : ""), len_(0), ended_(false), passthrough_(false), hasKey_(false), keyHash_(kFnvOffset)`
- L365 `BlockLog::~BlockLog()`
- L367 `void BlockLog::Key(const char* fmt, ...)`
- L381 `void BlockLog::Line(const char* fmt, ...)`
- L411 `void BlockLog::End()`
- L449 `const char* PatchDir()`
- L453 `void BringupMark(const char* name)`
