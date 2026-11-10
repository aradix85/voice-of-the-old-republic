# Contributing

This is Anouk's fork of Voice of the Old Republic (branch `bridge`). Its changes are written by Claude at her direction; the original mod takes contributions at [JeanStiletto/voice-of-the-old-republic](https://github.com/JeanStiletto/voice-of-the-old-republic). This file covers the dev setup, the inner loop, and the conventions that are not obvious from reading the code. The plan and the rules of the ongoing refactoring are in `docs/refactoring/STATE.md`.

## Before you start

Read `ARCHITECTURE.md` first. Then skim `docs/known-issues.md` for what is already on the backlog.

The player and owner of this fork is blind. Every workflow has to work with a screen reader and a keyboard: no mouse-only steps, no GUI-only tools, no "look at this screenshot".

## Dev setup

- **Windows 10 or 11** (the patch is a 32-bit Windows DLL injected into a 32-bit Windows game).
- **Visual Studio Build Tools** (2022 or later) with the C++ desktop workload (`cl.exe` for x86). The IDE is not required.
- **.NET 10 SDK** for `kdev`; it also builds the installer (net8.0-windows).
- **KOTOR 1**, English, version 1.0.3 (Steam or GOG). Its path goes into `kdev.toml` under `install`.
- **Lane Dibello's Kotor-Patch-Manager release** extracted somewhere. Path goes into `kdev.toml` under `[patch_manager].release`.
- **Python 3.12** for `tools/analysis/` (and `lizard` for its metrics).
- **NVDA** for testing: the mod speaks and brailles through nothing else.
- **JDK 21 + Ghidra 12.x** if you do reverse-engineering. Optional; `kdev` does not need it. See `docs/tools.md`.

## The inner loop

Edit a `.cpp` / `.h` under `patches/Accessibility/`, then:

```
kdev dev
```

That runs **clean → build → apply → launch**. The game starts with your changes injected. Patch logs land in `<install>/logs/patch-<utc>.log`. Tail with:

```
kdev logs --follow
```

Other commands:

- `kdev build` — compile only. Our sources build at warning level 4 and a warning fails the build.
- `pwsh -File tools/analysis/cppcheck.ps1` — cppcheck; exit code 0 means nothing outside the suppressions file.
- `pwsh -File tools/tests/run_unit_tests.ps1` — the unit tests. `-Mode asan` runs them under AddressSanitizer, `-Mode coverage` reports line coverage.
- `python tools/analysis/call_convention.py` — after adding or changing a call into the engine: checks that the typedef pushes as many bytes as the engine function pops. Needs a memory image of the game once (see the top of the script).
- `python tools/analysis/engine_twins.py` — after adding or changing a KOTOR 2 address: checks that it is the twin of the KOTOR 1 address beside it.
- `python tools/analysis/code_index.py` — regenerate `docs/llm-docs/code-index/` after changing code.
- `kdev apply` — install the most recent build into the game (game closed).
- `kdev launch` — start the game with the current install. Skip `--monitor`; it has a crash-after-inject bug. Read the patch log directly.
- `kdev kill` — force-terminate the running game (prefer Alt+F4 in-game).
- `kdev status` — sanity-check the project and game paths.
- `kdev analyze-dump <path-to-wer.dmp>` — registers, module list and faulting address from a Windows Error Reporting dump.

Every push to `bridge` is built by `.github/workflows/build.yml`, which runs the unit tests and publishes the mod and the installer as the pre-release `bridge-latest`.

## Conventions

### Code style

- Match what is already in `patches/Accessibility/`. Look at neighbouring files before introducing a pattern.
- File-name prefix groups the role: `engine_*` for read-side engine bindings, `menus_*` for per-screen GUI work, `audio_*` for audio, `combat_*`, `guidance_*`, `map_*`, `minigame_*`, `bridge_*`, etc.
- One job per file, about 600 lines or less, with a header comment that says the job. A module's shared internals go in `<module>_internal.h`, included only by its own files.
- Headers are kept narrow: one public namespace (`acc::interact`, `acc::viewpoint`, …) per pair, public functions in the header, everything else in the `.cpp`.
- Loader-lock-safe in `DllMain` only. Anything that loads DLLs, opens files or initialises COM is deferred to the first hook fire. See `core_dllmain.cpp`.
- Strings: never hardcode user-facing text in handler code. Add an `Id` to `strings.h` and its English text to `strings.cpp`; handlers call `strings::Get(Id)`. Log lines are English and stay out of the table.
- Engine-free logic (parsing, geometry, graph walks) goes in files that do not include engine headers, with doctest tests in `tests/unit/`.

### Comments

- Only write one when the *why* is not obvious: a hidden engine invariant, a workaround for a framework bug, a non-obvious offset. Engine knowledge stays.
- Do not restate what the code says, and no history (dates, "used to") in comments: that is what the commit log is for.

### Hooks

- Mid-function with register-source args. The framework's `source = "esp+X"` path has a known LEA-vs-MOV bug (`docs/upstream-prs.md`, PR-2); use `source = "ebx"` / `"edi"` / etc. instead.
- Find the cut point with `tools/ghidra-scripts/DumpBytes.java` (headless Ghidra).
- After landing a hook, log every trigger fire for the first sessions. Do not rate-limit diagnostic logs.

### Speech

- `acc::speech::Speak(text, interrupt)` sends one line to NVDA, spoken and on the braille display. Narrow text is Windows-1252 (`acc::speech::kGameCodepage`), like the English game's own strings.
- `acc::menus::SpeakIfChanged(channel, text, …)` dedups high-frequency listbox and focus lines per channel.
- Never silence a fallback. Generic "control N" / "row N" placeholders bypass dedup and throttling by design: if you have no name, speak the placeholder rather than nothing.
- Everything is text. The only sounds the mod plays are the swoop-race and turret cues.

### Engine reads

- Verify offsets against the live binary before trusting Lane's headers; SARIF or a decompile is the next checkpoint.
- One SEH layer, in the code that reads engine memory; no guard that re-checks what the caller already guarantees.
- Down-cast via vtable for typed reads (`vtable[20] = AsLabel`, `vtable[22] = AsButton`, …). See `engine_panels.cpp`.

## Testing

- **Unit tests** cover the engine-free logic and run in CI.
- **In-game testing** with NVDA is the real test: make the change, `kdev dev`, reproduce the scenario, listen, and check `<install>/logs/patch-*.log` for unexpected fire patterns.
- A change that cannot be tested in-game yet (a refactor, dead code removed) says so in its commit message.

## Reverse-engineering workflow

1. Look in `docs/llm-docs/re/` first: Lane Dibello's GOG-derived Ghidra database, headers, SARIF and PDF. The GOG bytes match Steam, so addresses are usable as-is.
2. If the function is not named, run `tools/ghidra-scripts/Decompile.java` via `analyzeHeadless.bat` for a C-like decompile.
3. Verify the bytes at the address with `tools/ghidra-scripts/DumpBytes.java` before writing the hook; the cut bytes go in `hooks.toml` and the framework matches on them at load time.
4. Land the hook with a logging-only handler first, confirm in-game that it fires when expected, then add behaviour.

## Commits

1. Work on `bridge`. One family of change per commit; a split is first a move-only commit, then a lean commit with the same behaviour.
2. `kdev build`, green unit tests and `tools/analysis/cppcheck.ps1` with exit code 0 before every commit.
3. Commit messages: a short subject (what and why), and a body when the diff does not carry the context.
4. If the change touches an engine offset, vtable index or hook address, name the source you verified it against (decompile, SARIF query, log line).
5. Add the change to the top section of `docs/CHANGELOG.md`.

## Where to find things

- **What an engine function does:** `docs/llm-docs/re/swkotor.exe.h` (Lane's headers), or `Decompile.java` for an address.
- **Where a function lives in this code:** `docs/llm-docs/code-index/`.
- **General accessibility-modding craft:** `ACCESSIBILITY_MODDING_GUIDE.md`.
- **Tool inventory and install paths:** `docs/tools.md`.
- **Kotor-Patch-Manager defects and the local fixes in the vendored copy:** `docs/upstream-prs.md`.
- **Historical investigation docs:** `archiev/`. They are frozen snapshots and need not match the current code.
