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
(8d2a198); kotor-tools tests (c7a9c41).

## Open

- room_topology.cpp: see the design below.
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
- Bridge: creatures in /objects; a party/equipment/inventory path;
  tag/resref in /objects for kotor_kaart matching; check what K2 needs;
  bridge test script (needs the game running).
- Static analysis pass; clean-build warning baseline (`kdev build --clean`).
- End: in-game checks (main menu without the updater, F5 does nothing; the
  untested features in brief.md), end report against the baseline, delete
  this file, short changelog note.

## Next design: room_topology.cpp (4,485 lines, 55 functions)

Held together by g_graph, kMaxNodes and ten helpers (AppendListEntry,
ClassifyCluster, ClassifyEdge, Degree, ExitPoint, FindDoorOnEdge,
HasGraphForArea, NoteExit, RenderDoorDirection, UFFind). Public API
(room_topology.h) unchanged:
- room_topology_internal.h: graph struct, g_graph, region kinds, shared
  constants and helpers.
- room_topology.cpp: Reset, BuildForArea (the pipeline), the public
  queries, DumpGraphToLog.
- room_topology_doors.cpp: door and transition snapshots with their retry,
  AttachLandmarksToDoors, FindDoorOnEdge, FindDoorNearPoint, ClassifyEdge.
- room_topology_merge.cpp: union-find and the merge passes.
- room_topology_shape.cpp: shape features, centroid axis,
  PassClassifyClusters.
- room_topology_words.cpp: octants, exit entries, door direction, corridor
  axis, far-end suffix: the words of a label.
- room_topology_label.cpp: the label per region kind (ClassifyAs*,
  demotions, junction, ClassifyClusterShape, ClassifyCluster).
- room_topology_diag.cpp: the four diagnostic log functions.
Tool first: tools/analysis/split_file.py, the exit_memory migration script
made general (function-to-module map; data follow their users; a free
comment block travels with the item after it).

## Rejected

- (none yet)
