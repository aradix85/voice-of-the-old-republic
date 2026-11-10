# Refactoring (fork aradix85) - working state

Temporary. Holds what a new session needs to carry on: decisions, method,
what is open, the next design. Not a log (git log is). When the refactoring
is done this file is deleted and the changelog keeps a short note.
The maker's earlier round (July 2026) is in `archiev/refactoring/`; its
rejections are advice only, Anouk decides.

## Decisions (Anouk, 2026-11-09)

- Scope: all of `patches/Accessibility/` (the maker's code too), the bridge,
  and `kotor-tools/` (outside the repo, local git).
- No upstream pull requests; upstream changes are taken over by hand.
- No duplicate code; time is no constraint.
- Split by job wherever possible, so the code is orderly and easy to check
  (she cannot program; Claude does the work). All seven rejections of the
  maker are reopened. Shared internals of a module go into
  `<module>_internal.h`, included only by that module's files; what one file
  uses stays local to it; shared mutable state lives in one `_state.cpp`.
- Target per file: one job, a header comment that says it, about 600 lines
  or less (data tables excepted).
- One mod for both games: keep the K1/K2 pattern, a K1 path and its K2 twin
  in the same file.
- Lean as well as split: per module first a move-only commit (checked with a
  line comparison), then a lean commit with the same behaviour. Lean: one
  SEH layer, in the code that reads engine memory; no guard that re-checks
  what the caller guarantees; no comment that restates the code, no history
  (dates) in comments. Engine knowledge in comments stays.
- Tests: unit tests for engine-free logic (doctest), Python tools with ruff,
  mypy and pytest, and a bridge test script against the running game.
- One branch, `bridge`; Anouk plays again only when the refactoring is done,
  so the in-game checks come at the end.
- Speech (Anouk, 2026-11-10): NVDA only, with braille, through the NVDA
  Controller Client; Prism goes, and with it the SAPI voices (urgent and
  compass). NVDA not running = silent. Out as well: positional speech and
  the exploration sounds (doors, walls, objects, beacons, collisions) where
  text already covers them. The minigame audio (swoop race, turret) stays.

## Rules

1. Verify a scan claim against the code before changing anything.
2. Measure calls and shared data before a split (split_plan.py).
3. One family of change per commit; `kdev build` with 0 warnings and green
   unit tests after every commit; push to `fork bridge`.
4. Never make a failure silent: a guard that makes a failure audible stays.
5. Record rejected changes below with the reason.
6. The repo's CLAUDE.md still applies (SEH constraint C2712, search for an
   existing helper before adding one).

## Method per file

1. `python tools/analysis/split_plan.py FILE --groepen --kern 6`.
2. Write the design below (files, job of each, what is shared).
3. Move-only commit, line comparison against the original.
4. Lean commit: duplicates, needless guards, restating comments; pure logic
   to engine-free files with unit tests.
5. `pwsh -File tools/tests/run_unit_tests.ps1`, kdev build, regenerate the
   code index (`python tools/analysis/code_index.py`).

Order by size: room_topology.cpp, menus_extract.cpp, minigame_turret.cpp,
menus_listbox.cpp, minigame_swoop_audio.cpp, prism.cpp, pad_input.cpp,
transitions.cpp, engine_area.cpp, unified_action_menu.cpp, combat_log.cpp,
then down the size list of analyse.py.

## Baseline for the end report (2026-11-09, e549e19)

334 files, 72,485 code lines (113,396 with comments), 2,615 functions; 212
functions above complexity 15. Without strings_*.cpp: 65,707 code lines,
40,617 comment lines, 581 __try blocks, 3,225 one-line guard returns, 437
comment lines with a date. 45 cloned blocks of 10+ lines.

## Done so far

exit_memory split into ten files and made lean (fccde30, c053a15); one
door-near-point helper (F1); five never-called functions removed (eb43055);
case folding in strfmt.h (F11, 504c33e); distance helpers next to Vector
(F10, 631eb0b); unit tests with save_points and shortest_paths taken out of
exit_memory (3c61b4d); generated code index (b957a29); updater removed
(8d2a198); kotor-tools tests (c7a9c41). room_topology split into eleven
files with tools/analysis/split_file.py (287ab2e), then made lean: F1's
SnapshotDoors loop, includes per file, history out of its comments.
menus_extract split into six files (936c320) and made lean: includes per
file, history out of comments, four cloned guarded row calls folded into
one GuardedRow. minigame_turret split into five files (bfbb7c4) and made
lean (2d11e0f); DriveSelectedPeg (700 lines, complexity 172) sits alone in
minigame_turret_peg.cpp: a P5 candidate, but only with an in-game test of
the turret game. menus_listbox split by panel into nine files (e7ea186),
each panel's spec defined in its file and declared extern, then lean
(1815b86). minigame_swoop_audio split into five files (879e9ff) and made
lean (8837b88): dead probe and co-pilot code out; the retired steering
ticks and the forwarding GetNavCueResrefForGame removed (8c1b675).
TickAccelpadCues and TickObstacleCues stay whole: P5 candidates, only
with an in-game test of the swoop race. bridge reviewed with kotor_kaart:
split into six files (1531ef0) and made lean (a37835f): one route table,
/state built on request instead of every 250 ms, engine-free bridge_http
with unit tests (exact token header, no NUL from a bad %XX), creatures and
tags in /objects; kotor-tools 98ad341 matches by tag and shows where
people are now. The two cooperate correctly: same frame (+X east, +Y
north), doors and placeables match on position, room kinds as kotor_kaart
expects. No resref: the engine has no template-resref reader and the tag
is enough.

Careful: never rewrite source files with PowerShell ReadAllText/WriteAllText
(it decodes UTF-8 and writes '?' for the dashes and umlauts in comments);
use the Python scripts, which read and write latin-1 byte for byte. And
never pipe a script that writes files through `Select-Object -First N`: it
stops the script once N lines are out. When redoing a split, delete only
the files the tool wrote: a module can already have files with the same
prefix (menus_listbox_picker.cpp).

## Open

- NVDA-only speech (decision above). A+B done: positional speech and the
  SAPI voices are out (1e25253). C done: speech.cpp talks to the NVDA
  Controller Client (third_party/nvda), Prism, Tolk and the SAPI
  workarounds are gone (2439893); installer, kdev apply and release.ps1
  ship nvdaControllerClient32.dll with its license and remove prism.dll
  and Tolk.dll on upgrade; the Windows voices option is gone (56062f2).
  Still to do:
  D. Exploration sounds out where text covers them (audio_cues NavCue door/
     wall/item/beacon/collision, audio_cue_player, guidance_beacon, the
     WallSounds toggle, view_mode/cycle_input/map_ui_cursor/passive_narrate
     cues): check each for a text equivalent first. Minigame audio
     (audio_bus, audio_loop, swoop, turret) stays.
  E. Docs pass after D: README.md and docs/README.*.md (settings rows,
     voices, 3D speech, sounds), controls-and-input, installer.md, tools.md,
     known-issues, llm-docs (audio-internals, action-menu-and-combat,
     game-flow).
  Note for Anouk at the end: with NVDA only, NVDA's "speech interrupt for
  typed characters" (Keyboard settings) cuts lines while a key is held
  (map cursor, turret fire key); the SAPI voices used to dodge that.
- room_topology: pure logic to engine-free files with tests (octant
  helpers; the bits E=0 counter-clockwise could become compass order N=0
  clockwise like engine_compass, which would also simplify
  kOctantEmitOrder). 39 of 55 functions are shared through the internal
  header: the pipeline really is one; accept.
- F12 engine yaw of a vector: `atan2(dy, dx) * 57.29...` written out in
  about 15 files with four spellings of the constant, often followed by
  EngineYawToCompass/CompassToSector/SectorString. One helper in
  engine_compass (bearing of a vector, and its compass word). kRadToDeg is
  defined in many files: one constant with it.
- Stale file names in comments (swoop_race.cpp, swoop_spatial_audio.h,
  turret_game.cpp and others from before the minigame_ prefix): one pass.
- Next file: pad_input.cpp, then transitions.cpp.
- F2b: share the save-variable code of exit_memory with discovery.cpp.
- F3 menus_chargen_feats / menus_powers_levelup: about 15 same-named
  functions, 22 cloned blocks; one chart-grid module (check K2 plans).
- F4 small list overlays (help, pad_quickmenu, combat_queue, examine_view,
  unified_action_menu): Open, BuildRows, SpeakRow, HandleInputEvent.
- F5 memory readers outside engine_reads (SafeRead in 4 files,
  ReadListBoxRowCount in 3, ReadCExoStringLocal, ReadButtonText).
- F6 PollWin32 key-edge polling in 9 files, against hotkeys.
- F7 cross-file clones: engine_reads_items/menus_store,
  cycle_input/interact_dispatch, engine_area_walls/guidance_pathfind,
  combat_query/examine_view, engine_actionbar/engine_radial.
- F8 chargen: AnnounceChainStepSuffix (4 files), AnnounceValueChange,
  RowPitchForCursorWarp (3 files).
- F9 minigames: HandleEnter/HandleExit/AnnounceEntry.
- Bridge: a party/equipment/inventory path; check what K2 needs (all
  but /health, /spoken, /state, /menu, say and key are KOTOR 1 only);
  live check of /objects with creatures and kotor.py kaart --personen
  (needs the game running with the new DLL).
- Static analysis pass; clean-build warning baseline (`kdev build --clean`).
- End: in-game checks (main menu without the updater, F5 does nothing; the
  untested features in brief.md), end report against the baseline, delete
  this file, short changelog note.

## Splitting a file

`python tools/analysis/split_file.py CONFIG.json` (format in its docstring;
the configs are not kept). After it, the project includes of the internal
header go to the files that use them (a small script did this for
room_topology; the compiler catches a miss).

## Rejected

- (none yet)
