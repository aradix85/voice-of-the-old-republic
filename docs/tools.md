# Tools

The tools this project uses on Anouk's machine: where they are, how to call them, and where each check stands. Update when a tool is added or a path changes.

## Game install (target)

**Star Wars: Knights of the Old Republic II — The Sith Lords** (Steam, Aspyr 1.0.2)
- **Install path:** `C:\steam\steamapps\common\Knights of the Old Republic II\` (`install_k2` in `kdev.toml`)
- **Executable:** `swkotor2.exe`, not encrypted on disk.
- **SHA-256 as shipped:** `6A522E71631DCEE93467BD2010F3B23D9145326E1E2E89305F13AB104DBBFFEF`, the hash `manifest.toml` declares as `kotor2_steam_aspyr`.
- **The file on disk is not the shipped one.** The original mod's installer put a static engine patch on it (SHA-256 `3A8AE7F9B802141F84891C49EC81D0F668670CE061C81D1A85C9F57AA685D8E0`); the code addresses are the same. Patch Manager still recognises the game through `kpm_install_state.json` in the install root, which records both hashes.

KOTOR 1 is not installed on this machine. `build/analysis/engine_image_k1.bin`, a memory image of its engine, keeps `call_convention.py` and `engine_twins.py` working for both games.

## Compiler

**Visual Studio Build Tools 2026** (version 18; compiler and Windows SDK, no IDE)
- **Install path:** `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\`
- **MSVC toolset:** 14.51.36231, 32-bit x86 through `VC\Auxiliary\Build\vcvars32.bat`
- `kdev build` finds it through `C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe`.

## Java

**Microsoft OpenJDK 21.0.12** in `C:\Program Files\Microsoft\jdk-21.0.12.101-hotspot\`; `JAVA_HOME` points at it. Ghidra 12 needs JDK 17 or newer.

## Ghidra (reverse engineering)

**Ghidra 12.0.4 PUBLIC** in `C:\Tools\ghidra_12.0.4_PUBLIC\`; headless analyzer `support\analyzeHeadless.bat`. Only the headless route is used: the GUI is not usable with a screen reader.

- **Project:** `kotor2` in `C:\Tools\ghidra-projects\`, program `swkotor2.exe`, made from `C:\Tools\k2re\swkotor2.exe` — a copy of the game's executable on a path without spaces (the patched file described above). There is no KOTOR 1 project on this machine, so `kdev re` and the scripts' defaults (project `kotor1`) do not work here: always pass the KOTOR 2 project.
- **Decompile a function:** in Git Bash, `KDEV_GHIDRA_PROJ=kotor2 KDEV_GHIDRA_PROGRAM=swkotor2.exe tools/ghidra-scripts/decomp.sh 0x<addr> [0x<addr> ...]`. It prints pure C, about seven seconds a call; give several addresses in one call. It passes `-noanalysis` (without that flag every call analyses the whole program again, minutes for KOTOR 2). Only one headless run at a time against one project (it takes a lock).
- **When the decompile hides something** — Ghidra sometimes leaves out the target of a call (`this` in ECX) — `python tools/re-scripts/disasm.py C:\Tools\k2re\swkotor2.exe 0x<addr>` shows the instructions.
- **A KOTOR 2 function by name:** `python tools/re-scripts/k2_gog_names.py C:\Tools\k2re\kotor2_gog_aspyr.db Class::Function` (also `--class Class` and `--offsets Class`). The database is `AddressDatabases/kotor2_gog_aspyr.db` from the current Kotor-Patch-Manager repository (2 MB, some ten thousand named functions of the GOG build; the vendored copy in `third_party/` is an older, nearly empty one). The script translates the GOG address to the Steam build; the result is a candidate that the decompile has to confirm, as its header explains.
- **The other headless scripts** in `tools/ghidra-scripts/` (each header gives its invocation): `DumpBytes.java` (bytes at an address, for a new hook's `original_bytes`), `FindCallers.java`, `FindFunction.java`, `FindScalar.java`, `ListFunctionsByName.java`, `ListSymbolsByName.java`, `PrintListing.java`. `Decompile.java` is what `decomp.sh` runs.
- **Reference data in the repo:** `docs/llm-docs/re/k2/` (among others `k2-functions.csv` and `k2-vtables.csv`); more scripts in `tools/re-scripts/`.

Lane Dibello's KOTOR 1 reverse-engineering bundle (Ghidra database, XML and SARIF exports, `swkotor.exe.h`) is not on this machine; `docs/llm-docs/re/` holds only `k2`. It is linked from his DeadlyStream thread, `https://deadlystream.com/topic/11948-kotor-1-gog-reverse-engineering/`. The recipes in `docs/llm-docs/sarif-cookbook.md` need it, and **jq** (installed through scoop, on PATH).

## Kotor-Patch-Manager

- **Vendored source:** `third_party/Kotor-Patch-Manager/` — version 0.6.0 with our fixes PR-1, PR-3, PR-4 and PR-5 (`docs/upstream-prs.md`). `KPatchCore` is compiled from it into `kdev` and the installer.
- **Runtime:** `[patch_manager] release` in `kdev.toml` names the folder with our rebuilt `KotorPatcher.dll`, which `kdev apply` copies into the game folder.
- **Upstream:** `https://github.com/LaneDibello/Kotor-Patch-Manager`. Its docs (`MULTI_VERSION_ARCHITECTURE.md`, `AddressDatabaseSystem.md`) are in the vendored copy under `docs/`.

## kotorsave (savegame inspector)

**`tools/kotorsave/`** — read-only CLI that pulls persisted object state out of a savegame: a patch log shows what the engine reports at runtime, the save shows how an object got into that state. It compiles source files from `third_party/KotOR_IO` (`https://github.com/LaneDibello/KotOR_IO`), which is not on this machine: clone it there before building.
- **Build:** `dotnet build tools/kotorsave/kotorsave.csproj` (output at `tools/kotorsave/bin/Debug/net10.0/kotorsave.exe`)
- **Save layout it walks:** `SAVEGAME.sav` (ERF) → `<module>.sav` (ERF) → `<area>.git` (GFF). `savenfo.res` beside it holds the save's name, area and playtime.
- **Usage:**
  - `kotorsave <save>` — save metadata plus the modules it still carries state for (the engine drops a module's state once you leave the planet).
  - `kotorsave <save> --module <module>` — summarise every entry in the module's `Door List`, with a plain-language verdict on whether the engine will still offer an Open action.
  - `kotorsave <save> --module <module> --tag <tag>` — dump every field of one object, nested lists expanded.
  - `--list "Placeable List"` switches to another GFF list; the error path prints the lists actually present.
- **`<save>`** is either the save directory or the `SAVEGAME.sav` inside it. It writes nothing.

## Analysis and test tools

The scripts find these at their default paths.

- **Cppcheck 2.22** in `C:\Program Files\Cppcheck\`. `pwsh -File tools/analysis/cppcheck.ps1` runs it with `tools/analysis/cppcheck_suppressions.txt`; `-Style` adds style findings and unused functions as a report.
- **LLVM 23.1** in `C:\Program Files\LLVM\` (clang-cl, clang-tidy, llvm-cov). `python tools/analysis/clang_tidy.py` runs clang-tidy with the checks in `.clang-tidy`; `run_unit_tests.ps1 -Mode coverage` builds the unit tests with clang-cl and reports line coverage.
- **The Microsoft compiler** itself: `pwsh -File tools/analysis/msvc_check.ps1 -Mode w4` (warning level 4 without a build) and `-Mode analyze` (the `/analyze` checker); `run_unit_tests.ps1 -Mode asan` for AddressSanitizer.
- **Python 3.12** with lizard, radon, ruff, mypy, pytest and coverage, for `tools/analysis/*.py`.
- **capstone and minidump** (pip) for `python tools/analysis/call_convention.py`: it compares the argument bytes of every engine function-pointer typedef with the `ret N` of the engine function itself, read from a memory image of the running game (`--from-dump` cuts one out of a process dump). One argument too many crashes the game. `python tools/analysis/engine_twins.py` checks the other half: that the KOTOR 2 address of each `Pick(k1, k2)` is the same method as the KOTOR 1 one, by comparing the calls, engine globals and strings the two functions use.
- `python tools/analysis/engine_constants.py` needs nothing beyond Python: it fails when an engine address or offset is defined in more than one file, and `--all` lists plain constants that share a name across files, for review.

Reports land in `build/analysis/`. Where the checks stood on 2026-11-11, unless a line gives its own date; anything above these numbers is new:

- `cppcheck.ps1`: exit 0. With `-Style`, 38 knownConditionTrueFalse findings remain, each a variable set inside a raw `__try`, which cppcheck does not follow. Left as they are: constVariablePointer and its relatives, useStlAlgorithm, uninitMemberVarNoCtor (structs the engine fills) and three false positives (bridge_describe containerOutOfBounds, cycle_state and skillinfo uninitvar).
- `clang_tidy.py`: 8 findings, all reviewed false positives (audio_loop, combat_log_parse, cycle_state, engine_area_names, guidance_autowalk, mod_settings_store, party_status, room_topology_absorb). Run it on every file a rewrite touches.
- `msvc_check.ps1 -Mode analyze`: 121. 105 are C28159 (GetTickCount is the live clock on purpose; `acc::tick::NowMs` is the time of the tick being dispatched), 7 C6385 and 3 C6054 are false positives, 4 C6262 are stack frames over 16 KB (engine_manager, map_ui_cursor, menus_chain_input, peek_description_item), 2 are in the Windows SDK.
- Unit tests (2026-11-26): 137 cases, 956 assertions. Line coverage of the files under test was 91% on 2026-11-11.
- `call_convention.py` (2026-11-26, `--game k1` and `--game k2`): 190 calls fit in KOTOR 1 and 198 in KOTOR 2, 11 not checked.
- `engine_twins.py` (2026-11-26): 168 pairs, 117 agree, 0 disagree, 3 mixed (the Abilities input handler, the inventory list filler and the client input handler; not in `engine_twins_reviewed.txt`), 48 unproven (small accessors with nothing to compare).
- `engine_constants.py`: no constant defined twice.
- `raw_try.py` (2026-11-26): 236 `__try` blocks outside `engine_reads`; 33 of them have no call inside and stay (several fields of one object under one guard, array walks, reads off a hook's stack frame).
- `analyse.py`: 479 files, 50,725 code lines, 2,444 functions, 135 above complexity 15, 3 cloned blocks of ten lines or more and none across files.

Splitting a file: `python tools/analysis/split_file.py CONFIG.json` (format in its docstring). Afterwards move the project includes of the internal header to the files that use them; the compiler names a missing one. When the module's public header is not `<module>.h`, fix the include in the internal header by hand, and move a constant that a shared function's default argument uses to the internal header yourself.
