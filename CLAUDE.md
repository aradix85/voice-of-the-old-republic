# Voice of the Old Republic — accessibility mod for KOTOR 1 and KOTOR 2

## Purpose
Accessibility mod for **Star Wars: Knights of the Old Republic** (BioWare 2003) and **Knights of the Old Republic II: The Sith Lords** (Obsidian 2004, Aspyr's 2015 rebuild), so blind players can play with NVDA (speech and braille, through the NVDA Controller Client). One build serves both games; KOTOR 1's hooks are in `patches/Accessibility/hooks.toml`, KOTOR 2's in `kotor2.hooks.toml`. This is Anouk's fork (aradix85, branch `bridge`) of JeanStiletto's Voice of the Old Republic: NVDA only, English only, keyboard only, text instead of exploration sounds, plus a local bridge.

## Chat answers
- Don't write raw memory addresses in chat answers. Use the function name, field name, or a short recognisable description instead ("PlayFootstep's field6_0x20 check", "the engine's natural early-out JZ"). Only include an address when it is itself the point — when distinguishing two adjacent hook candidates, or when a finding has to be re-typed into Ghidra or a hooks file. In code, hooks.toml comments and docs, addresses are encouraged.

## Permissions
- NEVER add broad PowerShell wildcard permissions like `Bash(powershell -Command:*)` or `Bash(powershell:*)` to settings.local.json - always use specific, scoped commands only

## Code standards
The conventions are in `CONTRIBUTING.md`. What it does not say:
- Before adding a helper, search for an existing one — duplicate utilities are a recurring failure mode here.
- Name a helper for what it DOES, never for the screen it is used on. `ReadCharSheetLabel` / `ReadEquipLabel` hid three copies of `engine_reads`' `ReadLabelTextAt`, because no grep could find them from either direction.
- The same applies to infrastructure: check whether a mechanism exists before proposing to build one. `acclog` already has `Trace` (line dedup), `BlockLog` (block dedup), `Once` and `Edge`.
- When fixing keyboard or focus handling, test the edge cases where the fix might interfere with the screen's normal behaviour.

## Non-obvious constraints
- **MSVC C2712: a function using `__try` cannot contain anything needing unwinding** — no objects with destructors, and no function-local `static` (its thread-safe init guard counts). Engine-reading code here is SEH-heavy by convention, so RAII and lazily-cached constants are unavailable across much of the patch. Workaround when you need an object anyway: declare it in an SEH-free CALLER and pass a pointer in — a pointer is trivially destructible, so the constraint never fires and the SEH code needs no restructuring (see `map_ui_cursor.cpp` passing `acclog::BlockLog*`)
- **Clocks.** A tick is one rendered frame (`core_tick` fans out from a detour on `CSWGuiManager::Update`), so the interval between ticks is whatever the framerate is. `GetTickCount` is right for a deadline ("have N ms passed") and for integration that updates its last stamp unconditionally. A rate (`delta / dt`) uses `acc::tick::NowMs()`, the stamp of the tick being dispatched: a near-zero interval falsifies a rate. `NowMs()` is valid only inside the tick dispatch; code reached from an input hook, a menu callback or a hotkey runs outside it and would read a stale value.

## Project reference

### Game install
- **KOTOR 2**, Steam: `C:\steam\steamapps\common\Knights of the Old Republic II`, `swkotor2.exe`, Aspyr 1.0.2. `kdev.toml` holds it as `install_k2`.
- **KOTOR 1** is no longer on this machine; `install` in `kdev.toml` still names its old path. Every `kdev` command that acts on a game therefore needs `--game k2`.
- **Config:** `swkotor2.ini` in the install root.
- **Engine:** BioWare Odyssey (an Aurora derivative).

### Project workspace
- **`tools/kdev/`** — internal dev CLI (.NET 10), drives the build → install → launch loop for the patch
- **`patches/Accessibility/`** — the patch source (hooks, menus, narration, audio, guidance, engine-interface modules), with `manifest.toml` and the two hooks files
- **`installer/KotorAccessibilityInstaller/`** — end-user installer (.NET 8 WinForms, self-contained single-file EXE); references KPatchCore directly. See `docs/installer.md`.
- **`.github/workflows/build.yml`** — on every push to `bridge`: builds the mod and the installer, runs the unit tests (plain and under AddressSanitizer), and publishes both as the pre-release `bridge-latest`, which the installer downloads the mod from
- **`tests/unit/`** — doctest unit tests for the engine-free code; run with `pwsh -File tools/tests/run_unit_tests.ps1` (`-Mode asan`, `-Mode coverage`)
- **`third_party/`** — vendored dependencies: `Kotor-Patch-Manager`, `nvda` (the controller client), `doctest`, `KeyMouseAccessibilityTest`.
- **`build/`, `logs/`** — generated; gitignored
- **`kdev.toml`** — project-level config consumed by `kdev`
- **`LICENSE`** — GNU GPL v3. All contributions ship under this licence.

### Documentation
Read the one that covers the work before diving in:
- **`docs/tools.md`** — the tools on this machine and how to call them (compiler, JDK, Ghidra and the reverse-engineering scripts, the analysis and test tools with where each check stands).
- **`docs/kotor2-port.md`** — the KOTOR 2 port: how far it is and what is still `Todo()` or `R()` at the top, then the findings per subject.
- **`docs/kdev-design.md`** — kdev's design decisions, project layout, subcommand contracts, exit codes.
- **`docs/controls-and-input.md`** — survey of KOTOR 1's keyboard and mouse controls.
- **`docs/installer.md`** — what the installer does: where the mod comes from, the flows, what lands in the game folder, the pins.
- **`docs/upstream-prs.md`** — Kotor-Patch-Manager (and K1CP) defects with our workarounds and the local fixes in the vendored copy; code comments cite its entries as PR-1 … PR-12.
- **`docs/known-issues.md`** — known limitations: what does not work or works differently, and why. No test plans or to-do lists.
- **`docs/CHANGELOG.md`** — release notes. The top section (`v0.9.1+bridge`) is this fork's; add every change there, in its overview and its topic subsection.
- **`docs/llm-docs/`** — reference material on the engine. See `docs/llm-docs/CLAUDE.md` for the index. `docs/llm-docs/code-index/` is generated from the source comments: `python tools/analysis/code_index.py`.
- **`archiev/`** — the original mod's historical investigation, design and progress docs. Search here if a code comment references a `docs/*-investigation.md` or `docs/navsystem-*.md` that no longer exists in `docs/`.

### Build/run
- **`kdev` itself:** `dotnet build tools/kdev/kdev.csproj -c Release` from the project root (output at `tools/kdev/bin/Release/net10.0/win-x64/kdev.exe`).
- **Mod build:** `kdev build` compiles `patches/Accessibility/` **incrementally** — per-TU object cache in `build/objcache/` with header dependencies tracked via `/showIncludes`, so only changed translation units (and TUs that include a changed header) recompile — then links and packages the `.kpatch` in `build/`. Our own sources build at `/W4 /WX`: a warning fails the build, and kdev prints the warnings of every file it compiles. `kdev build --clean` wipes the cache and recompiles everything. A single-file edit builds in seconds, a change to `strings.h` in about two minutes.
- **An interrupted build** leaves empty `.obj` files in `build/objcache/patch` (link error LNK1136): delete them and build again.
- **Long jobs in the background:** a cold `kdev build`, `kdev dev` and `kdev launch --monitor` can run for minutes; start them with `run_in_background`.
- **Mod install:** `kdev apply --game k2` calls `KPatchCore.PatchApplicator.InstallPatches` against the game's install and drops the `dinput8.dll` loader into the install root. It closes a running game first.
- **Mod launch:** `kdev launch [--monitor] --game k2` starts the game's exe directly; the loader auto-loads KotorPatcher. `--monitor` blocks on the game process and reports its real exit code.
- **Other:** `kdev status`, `kdev kill`, `kdev clean`, `kdev logs [--follow]`.

### Logs
- **Project logs:** `logs/` at project root (gitignored). `kdev` writes `kdev-<utc>.log`, `build-<utc>.log`, `dev-<utc>.log`.
- **Patch DLL logs:** `<install>/logs/patch-YYYYMMDD-HHMMSS.log` — one file per game session, written by the injected DLL via `acclog::Write` (`patches/Accessibility/log.cpp`). Name and line times are UTC. Tail with `kdev logs [--follow]` or grep the file directly.

### External references
- **Lane Dibello's GitHub** (Kotor-Patch-Manager, KotorMessageInjector, KotOR_IO): `https://github.com/LaneDibello`
- **PyKotor** (file-format Python library, HoloPatcher): `https://github.com/OldRepublicDevs/PyKotor`
- **xoreos** (open-source engine reimplementation, a cross-reference for engine internals and file formats): `https://github.com/xoreos/xoreos`
- **DeadlyStream** thread for Lane's KOTOR 1 reverse engineering, which links his Ghidra database: `https://deadlystream.com/topic/11948-kotor-1-gog-reverse-engineering/`
