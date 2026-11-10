# log.h (115 lines)

Source: `patches/Accessibility/log.h`

Logging primitives. The full log always goes to the session file; it also mirrors to OutputDebugStringA, but ONLY when a debugger is attached (an idle DBWin listener otherwise stalls the game's main tick — see log.cpp).

Conventions: every line carries a "Tag" prefix (first arg). Trace/Edge/ Once collapse high-frequency repeats while preserving the suppressed count — full fidelity without flooding. BlockLog does the same for multi-line snapshots (a panel walk, a manager-stack dump): identical consecutive blocks fold to a "(repeated Nx)" summary. No verbosity levels; dedup is structural.

## Declarations (in source order)

- L21 `void Init(HINSTANCE hinstDLL)`: Resolves \logs\patch- .log + \patches dir from the DLL's path.
- L24 `void Shutdown()`: Flush pending dedup counters + close the log.
- L27 `void Write(const char* tag, const char* fmt, ...)`: printf-style.
- L32 `void Trace(const char* tag, const char* fmt, ...)`: Dedup consecutive identical content per tag — emits one line then a "(repeated Nx more)" footer when the next distinct line arrives.
- L35 `void Once(const char* tag, const char* fmt, ...)`: Write only on first observation of `tag`.
- L39 `void Edge(const char* tag, int state, const char* fmt, ...)`: Write only when `state` differs from the previous call for `tag`.
- L42 `void WriteHex(const char* tag, const char* label, const void* bytes, size_t len)`: Splits long byte runs across multiple lines.
- L79 `class BlockLog`: Block-level dedup for multi-line snapshots that are re-emitted verbatim (a panel walk re-dumped on every rebuild, a manager-stack dump).
- L108 `void BringupMark(const char* name)`: Record a milestone in the bringup timeline.
- L112 `const char* PatchDir()`: Absolute path of \patches.
