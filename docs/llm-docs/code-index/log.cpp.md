# log.cpp (564 lines)

Source: `patches/Accessibility/log.cpp`

(From log.h.) Logging primitives. The full log always goes to the session file; it also mirrors to OutputDebugStringA, but ONLY when a debugger is attached (an idle DBWin listener otherwise stalls the game's main tick — see log.cpp).

Conventions: every line carries a "Tag" prefix (first arg). Trace/Edge/ Once collapse high-frequency repeats while preserving the suppressed count — full fidelity without flooding. BlockLog does the same for multi-line snapshots (a panel walk, a manager-stack dump): identical consecutive blocks fold to a "(repeated Nx)" summary. No verbosity levels; dedup is structural.

## Declarations (in source order)

- L28 `Locker()`
- L29 `~Locker()`
- L57 `DedupEntry* GetOrCreateEntry(const char* tag)`
- L80 `void RawWriteLocked(const char* tag, const char* content)`: --- Internal write (no dedup, must be called under lock) -----------------
- L136 `void FlushTraceLocked(DedupEntry* e)`: Flush a single Trace entry's pending suppression count.
- L145 `void FlushEdgeHoldLocked(DedupEntry* e)`: Flush an Edge entry's hold count.
- L174 `uint64_t Fnv1a64(const char* p, size_t n)`
- L183 `BlockEntry* GetOrCreateBlock(const char* tag)`
- L199 `void FlushBlockLocked(BlockEntry* e)`: Emit the "(repeated Nx)" summary for a folded run.
- L209 `void EmitBlockLinesLocked(const char* tag, const char* buf)`: Emit a '\n'-separated block buffer as one tagged line per segment.
- L227 `void SweepStaleLocked()`: Sweep all entries; flush any whose dedup window has gone stale.
- L249 `void Init(HINSTANCE hinstDLL)`
- L312 `void Shutdown()`
- L333 `void Write(const char* tag, const char* fmt, ...)`
- L347 `void Trace(const char* tag, const char* fmt, ...)`
- L384 `void Once(const char* tag, const char* fmt, ...)`
- L407 `void Edge(const char* tag, int state, const char* fmt, ...)`
- L445 `BlockLog::BlockLog(const char* tag) : tag_(tag ? tag : ""), len_(0), ended_(false), passthrough_(false), hasKey_(false), keyHash_(1469598103934665603ull)`
- L451 `BlockLog::~BlockLog()`
- L453 `void BlockLog::Key(const char* fmt, ...)`
- L471 `void BlockLog::Line(const char* fmt, ...)`
- L502 `void BlockLog::End()`
- L544 `const char* PatchDir()`
- L552 `void BringupMark(const char* name)`
