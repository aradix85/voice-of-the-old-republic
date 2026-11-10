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
- Language (Anouk, 2026-11-10): English only. The mod's six other
  languages (strings_*.cpp, language detection), the installer's
  translations and the translated READMEs go. The readme links (mod
  settings, installer) point to README.md of this fork, branch bridge.
- Input (Anouk, 2026-11-10 13:33): keyboard only. Everything about the
  mod's gamepad support goes, code and docs; KOTOR 2's own controller
  support (the game's) stays untouched.
- Leftovers (Anouk, 2026-11-10 14:12): clean them up right away rather
  than later. Installing tools is fine, test and analysis tools too.
- Comments (Anouk, 2026-11-10 14:20): only where they really add
  something, as long as Claude understands the code. The full trim of
  comments that restate the code happens per module during its lean.

## Rules

1. Verify a scan claim against the code before changing anything.
2. Measure calls and shared data before a split (split_plan.py).
3. One family of change per commit; `kdev build` with 0 warnings and green
   unit tests after every commit; push to `fork bridge`.
4. Never make a failure silent: a guard that makes a failure audible stays.
5. Record rejected changes below with the reason.
6. The repo's CLAUDE.md still applies (SEH constraint C2712, search for an
   existing helper before adding one).
7. Unused constants in the engine_offsets_*.h catalogs, the engine API
   addresses in audio_bus.h and documented struct layouts stay: verified
   engine knowledge, not dead code.

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
is enough. transitions split into five files (a820cec, which also holds
split_file.py's "tail" option for extern "C" hooks after the namespace)
and made lean (876e2a0): each file owns its state, one ResetAreaState,
dead spoken-position state out. Controller support out (keyboard only):
5a50c54 (pad modules, binding_hints, 116 strings, the pad-only helpers,
the K2 gamepad Quick Menu panel kind), cbeca56 (scancode lookups,
InteractNarratedTarget; this commit also carries the deletion of the
three controller docs), 660dd26 (docs), 0b2ca8d (comments, OpenPersonal,
ClearAllAndClose). The pad_input split and lean done just before
(9aaf284, 981529f, 2eb952e) went with it.
Cleanup round (cppcheck 2.22 and LLVM clang-cl installed): 32 header
functions without a caller and their leftovers out (e2c6366), unused
constants, types and globals (29795a3), cppcheck findings (ef19a74),
a real bug: the Swoop shift key name fell through to "Key help"
(e3dea96), clang -Wunused findings (9163413), EnsureSpeechInitialized
declared once in speech.h (5cab43f). Comments: current file names
(c76d646), English examples and terms instead of the maker's German
ones (93ed6f0). German identifiers renamed (bf2cadd: combat_strings
fields, combat_log rules, KindPlatz -> KindPlaza). English-only code
leftovers (5df2a82: German close/cancel words, multilingual skill-check
roots, German fallback captions, VK_OEM_3 named "Ö", "Talents" for
feats); "metres" in every string (the commit after).
engine_area split into six files (3841ae9) and made lean (6b0094f: one
GetGameObject lookup, one SEH layer around ReadCExoString, TryReadTag
was GetObjectTag). unified_action_menu split into six (1ed055a; split_file.py
fix 4c33467: a comment above a namespace line carried the line along) and
made lean (b75ade7). combat_log split into five (07754ad) and made lean
(0f244a4: BuildCompact from parts, English rule names). menus_chain split
into five (c52c540) and made lean (e240b6d); menus_chain_input.cpp (824)
stays for its turn on the size list. minigame_swoop_race split into five
(d61ecd3), a fix (92a3bbf: a race time still due at exit was spoken at the
start of the next race) and lean (e757a7e: one State{} reset, the offset
byte dump and acclog::WriteHex gone, one shift-gate table). Stale doc for
the llm-docs pass: swoop-k2-controls-and-jump.md still describes pad B ->
TryShift, per-game gate ladders and seven languages. map_ui_cursor split
into four (68545dd) and made lean (1cd1118: Tick from 534 lines to a flow
of named steps, one Nearest struct for the three hover scans, the map
transform offsets now in engine_area.h). kdev links through a response
file now (fe280ca: 274 objects passed the command-line limit).
menus_monitors split into five (a45c491) and made lean (5bbac2b: the
unused guiId column, FindListBoxChild and an old diagnostic gone).
minigame_pazaak split into five (e5a4708) and made lean (7a0cc5f: named
zones, helpers for the repeated speech lines). menus_pending: Drain's
784-line switch first became one function per op kind (0bfcc90), then a
split into four (204aab3) and lean (e5775cb: one Queue(), DrainActivate
as named steps). Left for an F-family: menus_crafting.cpp keeps its own
vtable[15] HandleInputEvent typedef next to menus_pending's
HandleInputEventOf. cycle_input split into three (81155d2) and made lean
(414bf1f, comments; F7 note: GuidancePreRollFor still mirrors
interact_hotkey's PreRollFor). menus.cpp made lean without a split
(8cf60b1, 1018 to 672 lines). menus_internal: namespace prep (94b0f40),
split into four (6d535b5), lean (37135ce5: one listbox drive with one
clamp, unused equip slot-button ids and the empty internal header gone,
menus_internal.h resolver comments shared). core_tick: keyboard recovery
split off (3f1af1cf; split_file.py now reads multi-line macros, 20141701,
but still parks an #undef next to its #define: move it back by hand),
lean (fe6e1108, comments only, phase order checked unchanged).
dialog_speech: Tick per panel (e8666d3d), split into three (6767b951),
lean (6ff3e3cb: skill-check marker engine-free with unit tests, shared
RecordTutorialLine/SilenceLine). CI note: the core_tick lean run failed
only on a transient HTTP 500 while publishing bridge-latest.
engine_radial: namespace prep (4d0bacca), diagnostics split off
(9148dd48), lean (4efbbed0: engine_reads instead of local readers,
shared action-list access). Left for an F-family (F5): the
CSWGuiInterfaceAction descriptor layout (kIfAction*) and a local
ReadInt32 are restated in engine_actionbar.cpp and engine_picker.cpp.
engine_panels: split into three (4da4778e; the existing internal header
was merged by hand, and includes_split.py also touched the old
engine_panels_state.cpp, reverted), lean (bc631a99: one vtable table
instead of seventeen detectors; CharGen got its PanelKindName row).

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
  D done: proximity cues, wall cache tick moved to wall_surfaces
  (f4c6019); beacon (Ctrl+- now speaks the route), cycling and Q/E cues,
  combat heartbeat (Anouk: not needed) and the audio-frame probe out, map
  edge and view-mode wall spoken (4d2371a). Left on purpose: minigame
  audio, footstep suppression when stuck (a silenced game sound, plus the
  spoken stuck directions), the installer's optional dsoal spatial-audio
  toggle (game audio). Still to do:
  T. English only, done: translated READMEs (4d07cdd); mod (6a7a263:
     strings.cpp is the English table, no language detection, one
     codepage speech::kGameCodepage); installer texts and readme links
     to the fork (ed2d156); installer assumes the English game, no
     localized TSLRCM or K1CP overlays (8c2dc36); translation docs and
     kdev combat-strings-extract out (the commit after); German in
     comments and names out in the cleanup round.
  I. Installer source (Anouk, 2026-11-10), done: no self-update, no
     --auto-update handoff, release.ps1 gone (9321977); the mod comes
     from this fork's bridge-latest, builds are named "bridge-<sha7>" and
     compared for equality, build.yml publishes the installer EXE too
     (0679714; check that CI run).
  E. Docs pass, done: README rewritten for the fork (2ea042a), CHANGELOG
     (bce53e6), CLAUDE.md, CONTRIBUTING.md, ARCHITECTURE.md (eabc45c),
     docs/installer.md new (old design notes to archiev/), known issues,
     controls, tools, kdev, controller port, upstream-prs.md reframed as
     "Framework defects and local fixes" (a6a6f9a), llm-docs (0df4e67),
     audio internals, K2 controller plan, a fork note on kotor2-port.md
     (its locale/Prism/beacon entries stay as history), issue and PR
     templates gone since the fork has issues off (089111d). archiev/ and
     the CHANGELOG's old entries stay as history.
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
- Next file:
  combat_queue.cpp (871), minigame_turret_peg.cpp
  (860), guidance_approach.cpp (840), peek_description.cpp (839),
  menus_modsettings.cpp (828), menus_chain_input.cpp (824) (sizes of
  2026-11-10 17:20, strings*.cpp excepted).
- F2b: share the save-variable code of exit_memory with discovery.cpp.
- F3 menus_chargen_feats / menus_powers_levelup: about 15 same-named
  functions, 22 cloned blocks; one chart-grid module (check K2 plans).
- F4 small list overlays (help, combat_queue, examine_view,
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
- Static analysis, left: cppcheck's 86 knownConditionTrueFalse findings
  (possible redundant guards) are not reviewed yet; take them per module
  during its lean. Command: cppcheck --enable=unusedFunction,style,
  warning,performance --platform=win32W -D_WIN32 -D_MSC_VER=1930
  --std=c++17 --language=c++ -I patches/Accessibility (plus
  --inline-suppr and the missingInclude suppressions) on
  patches/Accessibility; clang-cl -fsyntax-only -Wno-everything
  -Wunused-* per file.
- End: in-game checks (main menu without the updater, F5 does nothing; the
  untested features in brief.md), end report against the baseline, delete
  this file, short changelog note.

## Splitting a file

`python tools/analysis/split_file.py CONFIG.json` (format in its docstring;
the configs are not kept). After it, the project includes of the internal
header go to the files that use them (a small script did this for
room_topology; the compiler catches a miss, and it misses some: engine_app.h,
engine_reads.h, msg_router.h, party_cache.h). When the module's public
header is not <module>.h (combat_log: combat.h), fix the include in the
internal header by hand. A constant that a shared function's default
argument uses must go to the internal header by hand too (menus_chain:
kSortByOwnCy); the tool keeps it in the one file whose code uses it.

## Rejected

- cppcheck style findings left as they are: constVariablePointer and
  friends (noise in engine-pointer code), useStlAlgorithm (loops read
  better), uninitMemberVarNoCtor (POD structs filled by the engine),
  badBitmaskCheck on `kKindDeadEnd & 0xff |` (written that way to show
  the layout), view_mode shadow/duplicateAssign, and the false positives
  (bridge_describe containerOutOfBounds, cycle_state and skillinfo
  uninitvar).
- BTN_WAEHLEN / BTN_ABBRECHEN stay in a comment: they are .gui control
  tags, not prose.
