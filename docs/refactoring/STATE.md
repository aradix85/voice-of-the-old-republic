# Fork refactoring (aradix85) — State File

Persistent state of the second refactoring of this codebase, done in the
fork aradix85/voice-of-the-old-republic. Any session working on it reads this
file first and updates it before it ends. The chat transcript is not the
record; this file, git log and the code are. The maker's earlier effort (July
2026, phases 0-5) is archived in `archiev/refactoring/`; its END-REPORT is
worth reading for its lessons ("What was rejected, and why it matters",
"What actually finds defects"). Its rejections are advice, not decisions for
this fork: Anouk decides, and may put any of them back on the list.

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
- Split by job into modules wherever possible (Anouk, 2026-11-09 22:01):
  she cannot program and Claude does all the work, so the code must be as
  orderly and systematic as possible, and therefore easy to check. This
  overrides the maker's rejections of the room_topology, transitions and
  engine_radial splits; all seven of his rejections are reopened and
  re-evaluated with measurements when their file comes up (22:02). The
  maker's objection (a split publishes many internals through a header) is
  met with internal headers (`<module>_internal.h`), included only by the
  files of that module.
- One mod for both games stays (Anouk, 22:02): the existing K1/K2 pattern
  (offsets chosen per game with acc::off::Pick/Same, K2 branches beside
  their K1 counterparts) is kept, and a split keeps a K1 path and its K2
  twin in the same file.
- Target per file: one job, a header comment that says what that job is,
  roughly 600 lines or less (data tables such as strings_*.cpp excepted).
  "Per function" means per job, not one file per C++ function.
- Lean as well as split (Anouk, 22:12): duplicate code, needless defensive
  code and window dressing go. Each module gets two commits: first the move
  only (same lines, other place; checked with a line comparison), then a
  lean commit with the same behaviour. Lean means: one SEH layer, in the
  code that reads engine memory, and none above it; no guard that re-checks
  what its caller already guarantees; no comment that only restates the
  code, and no history in comments (dates, "used to"): git keeps that.
  Comments that carry engine knowledge (why an offset, what the engine does)
  stay. Rule 4 below still holds.
- Inside a module: what only one file uses stays local to that file
  (anonymous namespace); only what files share goes into the internal
  header, and shared mutable state is defined in one `<module>_state.cpp`.
- The changelog only records user-facing changes; a pure move or lean
  commit gets no entry.

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
5. One branch, one build: work directly on `bridge` (Anouk, 2026-11-09; the
   short-lived `refactor` branch was fast-forwarded into it and deleted).
   Anouk does not play until the refactoring is finished, so the in-game
   checks gather at the end; until then every commit must build green and
   pass the unit and Python tests.
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
- Without strings_*.cpp (tmp script defensief.py, 22:15): 65,707 code
  lines, 40,617 comment lines, 10,086 blank. 581 __try blocks, 3,225
  one-line guard returns (`if (...) return/continue/break;`), 437 comment
  lines with a date. Estimate for the lean commits: 10-20% fewer code lines
  and clearly fewer comment lines; measured per module before and after.

## Work plan

### P0 — Infrastructure
- [x] Branch `refactor`, this file.
- [x] Analysis scripts in `tools/analysis/` (lizard needed: pip install lizard).
  Run: `python tools/analysis/analyse.py <repo> <outdir>`, then
  `analyse2.py <outdir> <repo>/patches/Accessibility`.
- [ ] Clean-build warning baseline (`kdev build --clean`). The incremental
  build after P1 (110 TUs) gave 0 warnings.
- [x] `kotor-tools/` under local git (no remote; acf657e is the starting state).

### P1 — Remove what is not needed (YAGNI)
- [x] In-game updater (update_checker*, F5 in the main menu, its strings in
  seven languages, keybind row, README remark, code-index entries).
- [x] The five single-occurrence functions removed (defined, never called;
  not in hooks.toml or exports.def): combat_query ReadEffectCount,
  dialog_speech ShouldSuppressNpcLine, AreaObjectIterator::SnapshotSize,
  menus_crafting ReadListBoxSelection, probe_camera_distance SafeCallZoom
  (with its address constant and function type). The rebase-table entry
  for 0x006401D0 stays: it is engine data, not code.

### P2 — Tests and quality gates
- [x] Python (kotor-tools, c7a9c41): ruff, mypy and pytest configured in
  kotor-tools/pyproject.toml; 31 tests (kotor.py against a fake bridge,
  kotor_kaart helpers, integration with the installed game files). All clean.
  Run from kotor-tools: `.venv\Scripts\python.exe -m pytest -q`, `-m ruff check .`,
  `-m mypy kotor.py kotor_kaart.py kompas.py check_locks.py`.
  Same commit removed the duplication inside the scripts (one PyKotor guard,
  one compass module, gebied_info reused by kaart, one section printer).
- [ ] Bridge test script: with the game running, every path answers, the
  JSON has the expected fields, /state, /objects, /rooms and /overview agree
  with each other and with kotor_kaart.py.
- [ ] C++ unit tests: doctest (single header in third_party), a small test
  executable built by a script, for engine-free logic. First candidates: the
  save-variable formats (ACC_VISIT, ACC_EXIT), ShortestPaths, RoundWalk,
  compass/clock words. Needs that logic moved out of SEH-heavy files.
- [ ] Static analysis pass (MSVC /analyze or cppcheck) and triage.
- [ ] Code index: docs/llm-docs/code-index is hand-written and stale (about
  90 source files without an entry, 8 entries for files that no longer
  exist, door_announce.h.md misses ArmWalkThrough). Replace it with a script
  that generates it from the sources (header comment, declarations with
  line numbers), so it cannot drift; until then new files get no entry.
- [ ] Includes: each split module includes everything the original file
  did, through its internal header. Prune per file (include what you use)
  in the lean commit.

### P3 — Duplication (verify each before changing)
- [x] F1 Door lookup near a point: engine_area.h ForEachDoor and NearestDoor
  (templates) now serve exit_memory_doors.cpp, the exit-door search in
  exit_memory_overview.cpp and interact_dispatch's landmark-to-door. Left
  for the room_topology split: SnapshotDoors (it caps and logs) and
  AttachLandmarksToDoors.
- [ ] F10 Distance in the ground plane: seven helpers with four names
  (Dist2D, DistXY, HorizontalDistance, DistanceSquaredXY; plus DistXYSq,
  DistSq, DistanceSquared to check for 3-D) and `dx * dx + dy * dy` written
  out in about 43 places in 42 files. One pair DistXY/DistXYSq next to
  struct Vector (engine_offsets_types.h). Own commit across the code base.
- [x] F11 ASCII case folding: strfmt.h AsciiLower/AsciiUpper/ToLowerAscii/
  ToUpperAscii replace the copies in discovery.cpp, exit_memory_regions.cpp,
  menus_extract.cpp and the two tolower loops in bridge.cpp.
- [x] F2 exit_memory.cpp split by job (see "Done" below).
- [ ] F2b Share the save-variable code of exit_memory_visits.cpp (Persist,
  LoadVisited) and exit_memory_area_doors.cpp with discovery.cpp Persist.
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
- [ ] /objects without creatures. (Find-by-name done: `kotor.py zoek TEKST`,
  c7a9c41, searches /objects; creatures still missing there.)
- [ ] No party/equipment/inventory path: equipment and armour comparisons
  needed one-off PyKotor scripts on the save. One path or one script.
- [ ] kotor_kaart.py matches live objects to file objects by position
  (1.6 m); fragile. Expose tag/resref in /objects and match on that.
- [ ] Check what the KOTOR 2 side of the bridge really needs (YAGNI).

### Method for every split (P3 and the module work)

1. `python tools/analysis/split_plan.py FILE --groepen --kern 6`: per
   function its lines, the same-file functions it calls and the file-scope
   data it uses; with --kern the widely shared core is listed apart.
2. Write the module design into this file: the new files, the job of
   each, and what goes into the internal header.
3. Move code only (cut and paste, no logic change), keep the namespace,
   fix includes. One source file per commit.
4. `kdev build` with 0 warnings, the tests, and analyse.py before and
   after (code lines equal apart from headers and includes).
5. Update the code index (docs/llm-docs/code-index/<file>.md) for every
   new or changed file, and ARCHITECTURE.md where a module's place changes.

Order: our own exit_memory.cpp first (best known), then the largest files:
room_topology.cpp, menus_extract.cpp, minigame_turret.cpp, menus_listbox.cpp,
minigame_swoop_audio.cpp, prism.cpp, pad_input.cpp, transitions.cpp,
engine_area.cpp, unified_action_menu.cpp, combat_log.cpp, and on down the
size list of analyse.py. The duplication families (F1-F9) are taken along
when their files are split.

### Done: exit_memory.cpp split (move commit)

Ten files instead of one, public API unchanged (exit_memory.h). Line
comparison with the original: only includes, namespace lines, prototypes,
extern declarations, file header comments and section markers differ.
- exit_memory_internal.h (198): shared types (DoorName, Arrival,
  ExploreChain, Graph, Route), shared constants, extern state, and the
  prototypes of the helpers files share, grouped by defining file.
- exit_memory_state.cpp (21): the shared state.
- exit_memory_regions.cpp (204): region at a point, behind an exit, linked
  regions, map notes per region, door numbering, destination text.
- exit_memory_doors.cpp (93): door lookups near a point (F1 still to do:
  fold these together with interact_dispatch and room_topology into one
  engine_area helper).
- exit_memory_visits.cpp (210), exit_memory_graph.cpp (411),
  exit_memory_annotate.cpp (346), exit_memory_area_doors.cpp (192),
  exit_memory_overview.cpp (462), exit_memory_explore.cpp (231).
21 helpers became file-local; 25 are shared. Migration script (not kept):
%TEMP%\claude_split\split2.py; tools/analysis/skeleton.py prints a file's
non-function lines and function ranges, which the item list was made from.

### Design: exit_memory.cpp (2,212 lines, 65 functions, 54 file-scope data)

split_plan (--kern 6) puts BuildGraph, Dist2D, IndexOf, RegionAt and the
data g_area, g_current, g_visited, kFar in the core; the rest still forms
one group, held together by a few more helpers (RouteText,
TransitionSliverOwner, the door lookups, NumberedDoors). Those go into the
internal header too. Proposed modules, public API unchanged
(exit_memory.h):
- exit_memory_internal.h: the shared state (area, area key, save-var name,
  visited set, current and previous region, save-loaded flag) as one struct,
  the constants, RegionAt, RegionBehind, LinkedRegions, Dist2D, the Graph
  and Route types with their functions. Door lookups go to engine_area (F1).
- exit_memory_visits.cpp: visited regions in the save (Persist, RecordVisit,
  LoadVisited), OnAreaEntered, OnClusterCommitted, OnClusterSeen,
  MarkNearbySeen, Tick.
- exit_memory_graph.cpp: BuildGraph, ShortestPaths, ExitToward, the route
  functions, DoorSides, DoorBeyond, TransitionSliverOwner,
  UnreachableFlags, ListRegions.
- exit_memory_annotate.cpp: Annotate, HereList, RegionEntries,
  BehindDoorText, Destination, LandmarkRegions, door numbering,
  RegionLabelAt, CurrentRegionText, TargetWayInfo.
- exit_memory_area_doors.cpp: ACC_EXIT and ACC_SUM, TransitionDoorMemory,
  CurrentAreaSummary, TickAreaMemory.
- exit_memory_overview.cpp: CollectOverview, OverviewText, CollectLoot,
  PollOverviewHotkey, WarnIfBehindLock, RemindLootOnLeaving, FlushReminder.
- exit_memory_explore.cpp: the explore chain (Ctrl+Shift+N), WalkToNearestLoot
  (Ctrl+L), PollWalkHotkeys.

### P5 — Complexity hotspots (selective)
- [ ] Only where a split gives functions with short parameter lists and
  clear names; the maker's DriveSelectedPeg rejection is the precedent.

### P6 — Wrap-up
- [ ] Merge to `bridge` per verified family; end report; archive this folder.

## Rejected (do not raise again)

- (none yet in this effort; see archiev/refactoring/END-REPORT.md for the
  maker's list)

## Log

- 2026-11-09: P0 started, baseline measured, P1 updater removal (8d2a198),
  kotor-tools under git with tests (c7a9c41). All on `bridge` now. In-game
  check owed at the end: main menu responds, F5 there does nothing.
- 2026-11-09 22:00: Anouk chose splitting by job everywhere and reopened the
  maker's rejections; tools/analysis/split_plan.py added; exit_memory design
  written above.
- 2026-11-09 22:20: exit_memory.cpp split into ten files (move only), kdev
  build green with 0 warnings. Anouk asked for lean commits as well
  (duplication, defensive code, window dressing); decision recorded above.
- 22:45: lean commit for exit_memory: F1 door lookup, project includes moved
  from the internal header into the files that use them, history removed
  from four comments. Our own code turned out lean already (13 entry guards,
  all on values that can really be null or none; no SEH): the gains are in
  the maker's files. ClosedDoorNear and DoorNear now give the nearest door
  within 1.5 m instead of the first one found.
- 23:00: P1 single-use functions removed; F11 case folding in strfmt.h.
- Next: F10 (distance in the plane, codebase-wide), then the C++ unit-test
  harness (doctest) for the pure parts (save-var
  formats, ShortestPaths, RoundWalk), then room_topology.cpp.
