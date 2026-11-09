# Fork refactoring (aradix85) — State File

Persistent state of the second refactoring of this codebase, done in the
fork aradix85/voice-of-the-old-republic. Any session working on it reads this
file first and updates it before it ends. The chat transcript is not the
record; this file, git log and the code are. The maker's earlier effort (July
2026, phases 0-5) is archived in `archiev/refactoring/`; its END-REPORT is
required reading, above all "What was rejected, and why it matters" and
"What actually finds defects".

## Decisions

- Scope: the whole of `patches/Accessibility/` (the maker's code included),
  the bridge, and the helper scripts in `kotor-tools/` (outside this repo).
  Decided by Anouk 2026-11-09.
- The fork no longer aims at upstream pull requests. Upstream releases are
  read and useful changes taken over by hand (cherry-pick or re-implement);
  conflicts are accepted. The in-game updater is removed (it would overwrite
  the fork's DLL with the maker's).
- Tests: both kinds. Unit tests for engine-free logic, plus a bridge test
  script that checks the running game. Python helpers get ruff, mypy and
  pytest.
- No duplicate code (Anouk): duplication is the first target. Time is not a
  constraint.

## Rules of engagement

1. Verify every scan claim against the code before changing anything. The
   maker found the scan's framing wrong in six of seven cases; similar names
   are not the same shape.
2. Before splitting a file, measure the cross-call matrix and the shared
   state. A split that publishes many internals through a header is worse
   than one cohesive file (the rejected room_topology split).
3. One family of change per commit, behaviour-preserving unless the commit
   says otherwise. `kdev build` green with 0 warnings after every commit.
4. Never make a failure silent: a guard whose job is to make a failure path
   speak stays.
5. Work on branch `refactor` (cut from `bridge` at e549e19). Merge into
   `bridge` only after a family is play-tested; Anouk plays the `bridge` build.
6. Record every rejected change below with its reason, so it is not raised
   again.
7. CLAUDE.md of the repo still applies (no tables in chat, blind developer,
   SEH constraint C2712, search for an existing helper before adding one).

## Baseline (2026-11-09, branch point e549e19)

Measured with `tools/analysis/analyse.py` and `analyse2.py` (lizard + own
clone and name scans):
- 334 files, 72,485 code lines (113,396 with comments), 2,615 functions.
- Median function: 11 lines, cyclomatic complexity 4. 212 functions above
  complexity 15, 37 above 50; 55 functions above 100 lines.
- Highest complexity outside the string tables: swoop_race::TickAccelpadCues
  (207), turret_game::DriveSelectedPeg (172, decomposition rejected by the
  maker), change_detector::Tick (116), swoop_race::TickObstacleCues (112),
  map_ui_cursor::Tick (109), input_poll::PollHotkey (101),
  menus::pending::Drain (101), OnClientHandleInputEvent (95).
- The seven strings_*.cpp Get() switches (914 cases each) and the seven
  EffectName* functions are data written as code; they dominate the
  complexity numbers but are not logic.
- Clones of 10+ identical lines: 45 blocks, 28 across files. No include
  cycles between headers. 5 functions whose name occurs only once.
- Our own additions since v0.9.1: 54 files, +6,047 lines, mostly
  exit_memory.cpp (+2,212) and bridge.cpp (+1,015).

## Work plan

### P0 — Infrastructure
- [x] Branch `refactor`, this file.
- [ ] Analysis scripts in `tools/analysis/` (lizard needed: pip install lizard).
- [ ] Clean-build warning baseline (`kdev build --clean`).
- [ ] Put `kotor-tools/` under local git (it has no history today).

### P1 — Remove what is not needed (YAGNI)
- [x] In-game updater (update_checker*, F5 in the main menu, its strings in
  seven languages, keybind row, README remark, code-index entries).
- [ ] The five single-occurrence functions: combat_query ReadEffectCount,
  dialog_speech ShouldSuppressNpcLine, AreaObjectIterator::SnapshotSize,
  menus_crafting ReadListBoxSelection, probe_camera_distance SafeCallZoom.
  Verify each (hooks.toml, exports.def, function pointers) before removal.

### P2 — Tests and quality gates
- [ ] Python (kotor-tools): ruff, mypy, pytest. kotor.py against a fake
  bridge server; kotor_kaart.py against the installed game files.
- [ ] Bridge test script: with the game running, every path answers, the
  JSON has the expected fields, /state, /objects, /rooms and /overview agree
  with each other and with kotor_kaart.py.
- [ ] C++ unit tests: doctest (single header in third_party), a small test
  executable built by a script, for engine-free logic. First candidates: the
  save-variable formats (ACC_VISIT, ACC_EXIT), ShortestPaths, RoundWalk,
  compass/clock words. Needs that logic moved out of SEH-heavy files.
- [ ] Static analysis pass (MSVC /analyze or cppcheck) and triage.

### P3 — Duplication (verify each before changing)
- [ ] F1 Door lookup near a point: exit_memory DoorNear, ClosedDoorNear,
  TransitionDoorNear, LockedDoorPositions; interact_dispatch landmark-to-door;
  room_topology AttachLandmarksToDoors. One engine_area helper.
- [ ] F2 exit_memory.cpp (about 2,100 lines, seven jobs): visited regions
  and save variables, region graph and routes, doorway slivers, region
  annotations and door contents, overview, loot, explore walk. Split by job
  after measuring the cross-call matrix; share the save-variable code with
  discovery.cpp Persist.
- [ ] F3 menus_chargen_feats.cpp and menus_powers_levelup.cpp: about 15
  functions with the same name and 22 cloned blocks (chart navigation,
  binding, description, cell status). One shared chart-grid module. Check
  KOTOR 2 plans first (the K2 powers screen is reworked).
- [ ] F4 Small list overlays: help, pad_quickmenu, combat_queue,
  examine_view, unified_action_menu share Open, BuildRows, SpeakRow,
  HandleInputEvent, ForceDisarm, HandleNavCode. Compare shapes first.
- [ ] F5 Memory readers outside engine_reads: SafeRead (4 files),
  ReadListBoxRowCount (3), ReadCExoStringLocal and ReadButtonText
  (engine_actionbar, engine_radial), LogState.
- [ ] F6 PollWin32 key-edge polling in 9 files: check against hotkeys.
- [ ] F7 Cross-file clones: engine_reads_items/menus_store,
  cycle_input/interact_dispatch, engine_area_walls/guidance_pathfind,
  combat_query/examine_view, engine_actionbar/engine_radial.
- [ ] F8 Chargen attributes/skills: AnnounceChainStepSuffix (4 files),
  AnnounceValueChange, RowPitchForCursorWarp (3 files).
- [ ] F9 Minigames: HandleEnter/HandleExit/AnnounceEntry (swoop race,
  turret, pazaak).

### P4 — Bridge and map script
- [ ] /objects without creatures; kotor.py has no find-by-name (handles had
  to be dug out of raw JSON with a one-off script, 2026-11-09).
- [ ] No party/equipment/inventory path: equipment and armour comparisons
  needed one-off PyKotor scripts on the save. One path or one script.
- [ ] kotor_kaart.py matches live objects to file objects by position
  (1.6 m); fragile. Expose tag/resref in /objects and match on that.
- [ ] Check what the KOTOR 2 side of the bridge really needs (YAGNI).

### P5 — Complexity hotspots (selective)
- [ ] Only where a split gives functions with short parameter lists and
  clear names; the maker's DriveSelectedPeg rejection is the precedent.

### P6 — Wrap-up
- [ ] Merge to `bridge` per verified family; end report; archive this folder.

## Rejected (do not raise again)

- (none yet in this effort; see archiev/refactoring/END-REPORT.md for the
  maker's list)

## Log

- 2026-11-09: P0 started, baseline measured, P1 updater removal done on
  `refactor`.
