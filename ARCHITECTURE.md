# Architecture

The mental model of this fork of Voice of the Old Republic. Read this before making non-trivial changes.

## From the outside in

- **The installer** (`installer/KotorAccessibilityInstaller/`, .NET 8 WinForms, a self-contained single-file EXE) downloads `Accessibility.kpatch` from this fork's `bridge-latest` release and installs it with Lane Dibello's `KPatchCore`, together with the bundled files: the loader, the patch runtime, NVDA's controller client, the widescreen patch and, on request, the K1 Community Patch (via HoloPatcher).
- **In the game folder**, `dinput8.dll` is a proxy loader that starts `KotorPatcher.dll` (the Kotor-Patch-Manager runtime) with the game. The runtime applies the hooks declared in our patch and loads `patches/accessibility.dll`. `nvdaControllerClient32.dll` sits next to it.
- **Inside swkotor.exe**, our DLL hooks engine code, reads game state, speaks and brailles every line through NVDA, plays the minigame cues, and serves the local bridge.

The dev CLI (`kdev`) automates build, install and launch beside this. `.github/workflows/build.yml` builds the mod and the installer on every push to `bridge` and publishes them as `bridge-latest`.

## Pieces in this repo

### `patches/Accessibility/` — the patch DLL

Native C++17 (MSVC x86), built by `kdev build` into a `.kpatch` archive. Files are organised by job, not by screen; a module's shared internals live in `<module>_internal.h`, included only by its own files. The prefixes:

- **`core_*`** — DllMain and the per-tick dispatcher (`core_tick.cpp`). Loader-lock-safe: no DLL loads, file I/O or COM in `core_dllmain.cpp`.
- **`engine_*`** — read-side bindings to engine memory: struct offsets, function-pointer wrappers, vtable downcasts. The project's "header file for the engine".
- **`speech`, `speech_clipboard`, `strings`, `log`** — infrastructure. `speech` sends each line to NVDA (spoken and on the braille display); `strings` holds every spoken text, one English string per `Id`; `log` writes the patch log.
- **`menus_*`** — per-screen GUI accessibility: chargen, character sheet, journal, store, equipment, save and load, level-up, workbench, party selection, mod settings. They share the listbox, chain and extraction helpers.
- **`room_topology_*`** — cuts an area's walkmesh into regions and describes them: shape, label, exits.
- **`exit_memory_*`** — what lies behind each door, routes between regions, the exploration overview (Ctrl+N), the explore walk (Ctrl+Shift+N), and the save variables that keep visited regions and door destinations.
- **`discovery`, `transitions`, `door_announce`, `locked_recall`** — which objects you have found, region and area announcements, doors opening, and how a locked door or container opens.
- **`cycle_*`, `qe_ring`, `filter_objects`, `same_name_suffix`, `narrated_target`** — the Q / E and `,` / `.` cycles and the focused-object model.
- **`guidance_*`, `interact_dispatch`** — walking for the player (autowalk, rescue walk, sidestep), path finding over the region graph, and spoken route descriptions.
- **`combat_*`** — combat-log narration (English anchors in `combat_strings.cpp`), the action queue, combat queries.
- **`camera_*`, `view_mode`, `announce_facing`, `spatial_wall_surfaces`, `wall_probe`** — orientation: facing, turning to exits, view mode, the wall cache.
- **`map_*`** — the map screen: cursor, map notes, user markers.
- **`party_*`** — party status (Ctrl+H), leader changes.
- **`pad_*`** — the controller driver (KOTOR 1 has no gamepad code; the mod reads XInput itself) and its action and quick menus.
- **`minigame_*`, `floor_puzzle`** — turret, swoop race, pazaak and puzzles. The swoop and turret cues are the only sounds the mod plays.
- **`audio_*`** — the audio bus for those cues, looping sources, pitch, and footstep suppression when the player is stuck.
- **`bridge_*`** — the local HTTP bridge (see below).
- **`hotkeys`, `input_*`, `key_inject`, `msg_router`, `help`, `tutorial_*`, `binding_hints`** — the input layer, key bindings, F1 help and the tutorial hints.
- **`diag_*`, `probe_*`** — diagnostics: logging only, no behaviour.

`hooks.toml` (and `kotor2.hooks.toml`) declare the engine functions we detour. Most hooks are mid-function with register-source arguments: the framework's stack-source path has a known LEA-vs-MOV bug (`docs/upstream-prs.md`, PR-2).

### `tests/unit/` — unit tests

Doctest tests for the engine-free code (HTTP parsing of the bridge, shortest paths, save points, geometry and text helpers). `pwsh -File tools/tests/run_unit_tests.ps1` builds and runs them; CI runs them on every push.

### `tools/`

- **`tools/kdev/`** — the dev CLI (.NET 10): `status`, `build`, `apply`, `launch`, `dev`, `kill`, `logs`, `analyze-dump`, and offline walkmesh analyses.
- **`tools/analysis/`** — measures the patch source (function length and complexity, clones, include cycles), plans and performs file splits (`split_file.py`), and generates `docs/llm-docs/code-index/` (`code_index.py`).
- **`tools/ghidra-scripts/`** — headless Ghidra scripts for decompiles and byte dumps.

### `third_party/`

- `Kotor-Patch-Manager/` — Lane Dibello's hook framework, with local fixes (see `docs/upstream-prs.md`).
- `nvda/` — the NVDA controller client (x86 DLL, header, LGPL-2.1 license).
- `doctest/` — the unit-test framework.

## Key design decisions

### Hook vs. poll

The default is **polling** from the per-tick dispatcher. Hooks are for paths where we must consume input, change engine state, or catch a one-shot event that cannot be observed otherwise (a combat log line, a focus change, an area load): hooks for control flow, polling for observation.

### Read engine state directly; never mirror it

Struct offsets are in `engine_offsets*.h`; vtable indices and downcast helpers in `engine_panels.cpp` / `engine_reads*.cpp`. If you need a field the engine renders, find the offset and read it rather than keeping a parallel copy that can drift. One SEH layer, in the code that reads engine memory.

### Everything is text, through NVDA

Every spoken line goes through `acc::speech::Speak(text, interrupt)`: NVDA speaks it and shows it on the braille display. There is one speech path, no voice priorities and no positional audio; without NVDA the mod is silent. What the original mod told with sounds (walls, doors, objects, beacons) is said in words. Spoken text lives in `strings.cpp` (English, Windows-1252, like the English game's own strings), so handler code never holds a literal. Log lines are English and stay out of the table.

### The bridge answers on request

The bridge (`bridge_*`) listens on 127.0.0.1 with a per-session token written to `bridge.json` next to `swkotor.exe`. A request is queued and answered on the game thread within the next ticks; nothing is read from the game while no tool asks. `bridge_http.*` (parsing, JSON) is engine-free and unit-tested. The endpoints are listed in `bridge.h`.

### Framework changes stay local and recorded

If the patch framework must behave differently, work around it in our code or patch the vendored copy in `third_party/Kotor-Patch-Manager/`, and record the defect and the fix in `docs/upstream-prs.md`. No pull requests are sent from this fork.

## What this codebase is not

- Not a content mod. It does not edit `.dlg`, `.utc`, `.uti`, `.2da`, `.nss` or `dialog.tlk`; it reads game state and adds accessibility on top.
- Not a cross-engine port. It targets the original Odyssey binaries: KOTOR 1 1.0.3 (Steam and GOG, English) and, partly, Aspyr's KOTOR 2.
