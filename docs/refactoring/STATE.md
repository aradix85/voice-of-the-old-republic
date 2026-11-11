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
- View mode (Anouk, 2026-11-10 22:10): goes. She walks with
  Ctrl+Shift+N and autowalk and does not use it.

## Rules

1. Verify a scan claim against the code before changing anything.
2. Measure calls and shared data before a split (split_plan.py).
3. One family of change per commit. After every commit: `kdev build` (our
   sources build at /W4 and a warning fails the build), green unit tests,
   `pwsh -File tools/analysis/cppcheck.ps1` with exit 0; push to
   `fork bridge`.
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
5. `pwsh -File tools/tests/run_unit_tests.ps1`, kdev build, cppcheck.ps1,
   regenerate the code index (`python tools/analysis/code_index.py`).

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
lean (2d11e0f). menus_listbox split by panel into nine files (e7ea186),
each panel's spec defined in its file and declared extern, then lean
(1815b86). minigame_swoop_audio split into five files (879e9ff) and made
lean (8837b88): dead probe and co-pilot code out; the retired steering
ticks and the forwarding GetNavCueResrefForGame removed (8c1b675).
bridge reviewed with kotor_kaart:
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
byte dump and acclog::WriteHex gone, one shift-gate table). map_ui_cursor split
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
as named steps). cycle_input split into three (81155d2) and made lean
(414bf1f, comments). menus.cpp made lean without a split
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
shared action-list access).
engine_panels: split into three (4da4778e; the existing internal header
was merged by hand, and includes_split.py also touched the old
engine_panels_state.cpp, reverted), lean (bc631a99: one vtable table
instead of seventeen detectors; CharGen got its PanelKindName row).
combat_queue: split into three (272ada84), lean (eeed8dd2: one queue
walker, engine_reads for the field reads, one guarded RemoveLastAction).
minigame_turret_peg: DriveSelectedPeg as named steps (f0d58a50), aim
assist out to minigame_turret_assist.cpp (b56066ec; split_file.py had
written the core to minigame_turret.cpp, restored, and now keeps the
source name and never overwrites, 19ce52cf), lead solve engine-free in
minigame_turret_lead.cpp with five tests (79d07af0).
guidance_approach: stall rescue split off (93ae4706), lean (cbe3dcae:
shared release and stall-restart helpers, thresholds in their files, no
state file). F12 note: SpeakWayBlocked writes out the yaw-to-compass
chain too.
peek_description: item descriptions split off (371fb862), lean (60556349:
one block cursor step, item-block cache in the item file, HandleShiftArrow
as a dispatch, journal address and offset in the engine catalog).
menus_modsettings: glossary and list input split off (417505e0), lean
(0fc9fb0c: persist key in the option table, shared ResetState and
StepListCursor, Enter dispatched by row kind, no state file).
menus_chain_input: Enter and Left/Right/Esc split off (c9e85ee1), lean
(3bee77cc: Enter routes as a table of named functions, named steps for
Up/Down and Esc).
hotkeys: defaults, user rebinding and names split off (66774adf), lean
(92909f7e: UsesKey/ModsSatisfy, Index(), kAnyMod, Persist(), no state file).
engine_input: mouse guard and acquire split off (f48930cc), lean (f5d6c509:
ExoInput(), one SEH CallSetActive with an Ok check).
minigame_swoop_audio_accelpad: tick in named steps (dc078509), detours and
assist split off (cd5dc356), lean (f026304e).
interact_dispatch: DispatchInteract in named steps (1f96b5ed), bare keys to
interact_bare_keys.cpp (4a37be23), one PreRollFor shared with
cycle_input_focus (2adbd326, the GuidancePreRollFor clone).
examine_view: reads and rows split off (bba86317), lean (c8af177b: the
equipment reader of combat_query, engine_reads for the memory reads).
engine_reads_items: descriptions and actions split off (f571fd25), lean
(edf9a39e: TryRead*, GetFeatRow/GetSpellRow/ReadRowStrRefText shared with
examine_view).
combat_query: the leader-change poll moved to party_leader_announce
(c90e8703); lean, 585 to 282 lines, the HP, effect-icon and effect-type
reads in engine_reads and shared with the examine view (bef48dc3).
strfmt: FixedText, the one bounded printf-append (c421d637).
room_topology_doors: lean, 565 to 409 lines, no split (8afeed90).
log: first a unit test of what reaches the file (b278cadd), then lean,
563 to 464 lines, no split; 91% of its lines run in the test.
Leftovers: the audio probe's two hotkey actions, comments that named
interact_hotkey.cpp (7c216e29); docs name the current files (f1b0c7c3).
view_mode: narration and Enter split off (49c3e25e), lean (71ffbbfd);
removed with its listener hook and the wall-surface clustering
(82c9a9bf). viewpoint.h keeps what the rest asked it for: the camera
yaw and the player position.
filter: one ClassifyObject instead of four copies (189f655c).
input_pipeline: the in-world hook in named steps (abbba85d), 394 lines, no
split needed.
engine_subscreen: lean (3d3b3fb6: shared pause preamble, ReadUiState,
ReadSlots), 391 lines, no split needed.
menus: the SetActiveControl hooks to menus_setactive.cpp (b397b2d9), lean
(ae2c4396); stale menus.cpp references in comments and docs (3f937e2d).
focus_guard: movie-end guard and Big Picture warning split off (4e1ea2d3),
lean (ddcd8417: one poll step per job, ProcessExeName shared, the dead
AttachThreadInput branch gone); one ContainsNoCase in strfmt (f9a3c3f6).
engine_area_walls: wall records and SegmentCrossesWalkmesh to the
engine-free walkmesh_geometry (bd28beec), lean (e1c1f082: one mesh reader,
portal filter and same-room dedup engine-free with tests).
camera_orient: turns split off (0cd01306), lean (4ed0b3c9: the exit choice
engine-free in camera_orient_pick with tests).
room_topology_shape: Pass 3 in named steps (541819e6), moved to
room_topology_classify.cpp (a8b3789c), lean (97abb26c); English fallback
words "Door" and "Area" (36c31337).
CI: the publish step retries three times on GitHub's HTTP 500 (57c0afae).
tutorial_hints: dialog split off (86fcfb38), lean (071ac481: one TlkTexts
cache template). text_match engine-free with tests, out of engine_reads
(57c000cd).
minigame_swoop_audio_obstacles: TickObstacleCues in named steps
(dc21c158), bars split off (c49e10d9), lean (0405c4d2).
menus_extract_icons: equipment and speculative reads split off
(5192af4e), lean (6b697bf3). menus_crafting: rows split off (6e496ee3),
lean (db2ed1cf). camera_spin_guard: named steps (08b69d0a), lean
(46d71df5), 501 lines, kept whole. menus_editbox: lean (eff793ee, spec
table, 311 lines). menus_store: lean (686dfad0, 380 lines).
floor_puzzle: Tick in named steps (d6fc0304), the plate model engine-free
in floor_puzzle_plates (95adc230), lean (57f53880: one Visit struct,
tests).
engine_reads: ReadControlId replaces the raw id reads (F14 done) and
ReadEditboxText moves in from menus_editbox (dc8f308f);
ReadControlNameFields reads the tooltip at the per-game offset.
menus_extract_controls: lean (222d56de: TryAs over the engine_reads text
readers, PanelControls, TryEditbox through ReadEditboxText, the dead
TryTooltip gone).
audio_footstep_suppress: stuck probe in named steps (0cbc1240), moved to
audio_stuck_probe (14310482), lean (4ba10f9b: stuck window and the two
probe triggers engine-free in stuck_detect with tests, DistXYToSegmentSq
next to Vector shared with floor_puzzle_plates).
engine_reads: split into primitives, text, controls and objects
(367b60a0), lean (3f02ffd9: the Try* reads instead of private __try
blocks, ReadTextOrEmpty for label and button).
menus_dispatch: OnHandleInputEvent as a gate list of named steps
(2ae51bc7, 329 lines). Noted, not changed: the chargen feats and powers
grids return without the press tracker, unlike every other handler.
engine_picker: Drive in named steps and lean (f1d01f52, 348 lines).
minigame_swoop_audio_internal.h: layout and tuning constants to
minigame_swoop_audio_tuning.h (7eaf1d81).
Nothing above 600 lines is left except the catalog headers.
The 500-600 files, judged one by one: input_poll_router: PollHotkey as a
gate list of named steps (56de760d, 355 lines). room_topology: LookupAt
as ScanNodes, LogScan and the pick; BuildClusterLinks out of BuildForArea
(524 lines, kept whole). room_topology_label_kinds: ClassifyAsArea as
CollectAreaExits, RenderAreaExits and AxisCoversExits (ded00592).
engine_keymap: key tables and the [Keymapping] line parser engine-free in
engine_keymap_codes with tests (520ed847, f6541d58). spatial_wall_surfaces:
the anomaly diagnostics of BuildSurfaceDescriptors out (414 lines).
engine_player_party: lean (b9e67e5f: NpcObjectId, StatsFirstName, the
leader name as four sources in order; 386 lines).

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
  edge spoken (4d2371a). Left on purpose: minigame
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
- F12 done: kPi, kRadToDeg and kDegToRad are defined once
  (engine_offsets_types.h). VectorEngineYaw, VectorCompass and
  VectorSector in engine_compass (unit tests) replace
  `atan2(dy, dx) * 57.29...` and the chain after it in eleven files;
  kSectorSize and kHalfSector come from engine_compass.h;
  OctantFromVector of room_topology_words is SectorString(VectorSector).
  The turret files used two shorter spellings of the constant (57.2958,
  0.01745329): their angles change in the seventh digit. In-game check:
  Right Alt (facing and area), the compass word while the camera turns,
  the clock position on Q / E, "leads West" on a door, the route on
  Ctrl+-, "way blocked" with its direction, a region label that names a
  direction.
  kNoStrref (3 files) and kSkillSecurity (2) are in
  engine_offsets_values.h once.
- Goal (Anouk, 2026-11-10): split every file above 600 lines where that is
  possible; for files above 500 lines, judge whether a split into modules of
  things that belong together makes sense, and do it where it does.
- Next file, by size (2026-11-10 21:15, strings*.cpp excepted):
  nothing left to judge above 500. help is lean now (375: one Context
  enum, rows without stored text, Open and Close file-local; in-game
  check: F1 list with arrows, Home / End, Enter, Esc and F1 again, and
  Ctrl+F1 in the world and in a menu).
  menus_focus_k2 is lean now (239;
  its header is gone, the K2.Focus line keeps prev, id, vtable, caller;
  in-game check on KOTOR 2: the arrows in the main menu speak each button
  once and focus does not snap back). Judged
  and left whole: menus_listbox_picker (512): one concern, the equip
  picker and the workbench upgrade picker with their latches and
  monitors; a lean could give the two latches one struct. Judged and left whole: minigame_turret_peg 536, one
  task per tick that already has its named steps; what is left of
  DriveSelectedPeg is one pass over some twenty shared values, and its
  five diagnostic blocks could move behind a frame struct if the file is
  touched again. menus_focus is 422 after AnnouncePanelTitle got named
  steps. The catalog headers (engine_offsets_fields.h
  2318, engine_offsets_addresses.h 1060, engine_area.h 789) stay as they are.
- F13 done: one WriteJmpRel32, in code_patch.h. The three installers
  (mouse guard, keyboard resync, save guard) still each do their own
  VirtualProtect / write / restore / flush around it.
- StableFlag judged for camera_announce, party_leader_announce and
  door_announce: no fit. The camera debounces a sector with hysteresis
  and three reasons to speak, the leader announce is a tick countdown
  and a grace period after an area change, the door announce has
  one-shot windows. None is a boolean that has to hold still.
- discovery lost its migration of saves from before 2026-08-08 (the
  variable name once took the engine's spelling of the module): no save
  of this fork is that old. ToUpperAscii and AsciiUpper went with it.
- F2b done: what discovery and exit_memory shared around the save
  variables is in engine_scriptvar: ReadPlayerVar and WritePlayerVar on
  std::string (four static buffers of four sizes are gone, and a map-note
  summary over 512 characters no longer reads as empty), and
  PlayerVarsSettle, the one-second wait on a stable player creature that
  both had written out. The payloads differ (keys against points) and
  stay with their modules. In-game check: after loading a save, the
  visited regions, "visited" on doors and the , / . lists are as
  before the save.
- F3 done: the feat chart and the Force-power chart share
  menus_skillflow_grid (one Grid per screen, a Screen record for what
  differs); the two files went from 561 and 510 lines to 97 and 96,
  the column search (menus_skillflow_nav) has unit tests. In game,
  check at the next level-up: arrows, Home / End, Enter and Esc on
  the Feats screen; later the same on the Force powers screen.
- F4 judged, no shared model: help, combat_queue, examine_view and
  unified_action_menu share function names, not code. They differ in
  the input route (hotkey poll against the engine's input event), in
  when rows are rebuilt and in what Enter does; what is common is a
  clamped step of three lines. A shared model would cost more than it
  saves.
- F5 done: memory readers outside engine_reads. The SafeRead templates
  of camera_spin_guard and engine_navgraph are gone (TryReadFloat joined
  TryReadU32 / U16 / Ptr). The listbox rows: files
  walked the controls array of a CSWGuiListBox themselves; engine_reads
  now has ReadListBoxRow, ReadListBoxSelection, ReadListBoxSelectedRow
  and FindListBoxRow beside ReadListBoxRowCount. Step 1 uses them in
  dialog_speech, menus_keymap, menus_abilities, menus_crafting_rows,
  menus_listbox(_saveload), menus_chargen_layout, menus_journal and
  menus_inventory (in-game checks: a computer terminal's rows are spoken,
  the journal description on Enter, the key-mapping list, the skills
  list, Enter on an inventory item). Step 2 did the rest: the pickers
  (menus_listbox_equip, _workbench, _picker, _container),
  menus_monitors_dialog, menus_monitors_inventory, peek_description(_item),
  menus_extract_controls, menus_listbox_skillinfo, menus_skillflow_grid,
  menus_pending_workbench, menus_chain_tabs, menus_listbox_saveload. Most
  of these read the array without any guard, in per-tick monitors. The
  workbench picker's per-frame selection trace, marked temporary, is gone
  (its question was answered by the corner park). In-game checks: the
  equip picker (arrows speak the item and its place, Enter equips, Enter
  on the worn item takes it off), a container's item count and rows, the
  dialog replies, Shift+arrow descriptions on an item and in a menu, the
  workbench. menus_chain_build and menus_internal_listbox walk every row
  inside one guarded pass and stay; menus_powers_levelup takes the whole
  array through TryReadPtr. ReadCExoStringLocal (engine_actionbar) stays: it
  reads to the NUL and ignores the length field, which ReadCExoString
  does not.
  combat.cpp is lean (312 to 104): its listbox poll only wrote log lines
  and is gone, and the two debounces are one StableFlag (stable_flag.h,
  unit tests). In-game check:
  "Combat begins" and "Combat ends" once per fight.
- Class of an object (found with analyse2.py, same helper name in several
  files): ReadVtable and HasVtable are in engine_reads; some twenty files
  had the vtable read and compare written out, six of them without a
  guard, and four had a private ReadVtable, VtableOf or HasVtable. HasVtable is false for a
  vtable that is unknown in this game (0), which the hand-written
  compares were not. In-game check: chargen (class, portrait, attributes,
  skills, feats), the store, the journal, save and load, pazaak.
  The readers with a default are in engine_reads too: ReadU32Or0,
  ReadI32Or0, ReadPtrOrNull, ReadFloatOr0, with TryReadI32 and
  TryReadVector. They replace SafeReadPtr / U32 / Float / Vector of
  minigame_aim (some 100 uses in the swoop and turret files), ReadInt32
  and ReadPtr of engine_actionbar and engine_radial, ReadIntAt of the two
  pazaak files and SafeRead of the two camera probes. The unguarded
  ReadU32 is gone; its 8 callers read guarded now. In-game check: a swoop
  race, the turret game, pazaak (the board and the side-deck builder),
  the action menu.
  The two camera probes (probe_camera_state, probe_camera_distance: F12,
  Ctrl+F12 and Ctrl+F11, the last one wrote the camera distance) are out
  of the mod (Anouk, 2026-11-11), with their three hotkey actions and
  IsUserRebindable, which only existed to hide them.
  Looked for what the probes left unused: no function. A scan for
  constants that are defined and never used finds 95, nearly all in the
  catalog headers (engine_offsets_fields.h some 60, _addresses.h some
  20, _values.h 10, audio_bus.h 6): engine layout that was worked out
  and has no reader in the mod. Judged: they stay, the catalog is also
  the record of the engine, with the KOTOR 2 witnesses. Outside the
  catalog two unused constants went (kFollowerInvulnOffset,
  kJumpAirSec); four label offsets in menus_charsheet (fortitude,
  reflex, will, defense) are unused: the character sheet does not
  read those rows. acclog::Shutdown has no caller but its unit test:
  DllMain handles the attach only, so the log is never closed and a
  pending repeat count is lost at exit; wiring it into the detach
  needs care (a lock held by a thread that is already gone).
- F7: analyse.py now finds 3 cloned blocks of 10+ lines, none across
  files (the baseline had 45), and analyse2.py one near-copy pair. The
  helpers with the same name in two files are one now:
  DamageLevelStringId (combat_query), ReadAreaMiniGame (minigame_aim),
  acc::off::IsAt for the control embedded at an offset, and
  acc::engine::ResolveItemFromRow. ReadRowText stays three functions:
  they read different row shapes.
- One definition per engine constant. 21 addresses and offsets were
  written down in two or three files ("mirrors the declarations in ...,
  keep in sync"): the minigame headers repeated each other's struct
  offsets, CSWGuiInterfaceAction was described three times under two
  naming schemes (kIfAction*, kInterfaceAction*), the tutorial popup had
  a copy of engine_subscreen's pause addresses and pause code,
  kCSWCCreatureCombatModeOffset stood in three places. Each is in the
  catalog headers once now (engine_offsets_fields.h, _addresses.h,
  _values.h), with the comments of the copies merged; the tutorial popup
  pauses through engine_subscreen (SetMenuPauseBit, ResumeSoundMixer).
  tools/analysis/engine_constants.py checks it (exit 1 on a constant
  defined twice): run it with the other checks after every commit. Its
  --all mode lists plain constants that share a name across files, for
  review; it shows no real duplicate now.
  In-game check: the tutorial popups in the Endar Spire (the game pauses
  while one is up and resumes after), a swoop race and the turret game.
  minigame_aim is split, move only, into minigame_engine (the area's
  minigame, the object array, follower positions and boxes, the offset
  field) and minigame_magnet (the engine-free pull curve, unit tests).
  Lean done with it: the read of a virtual function (vtable, slot, null
  checks) was written out ten times; engine_reads has ReadVirtual(object,
  slotOffset) and ReadHandleInputEvent(object) now, used by
  minigame_engine, ReadAurObjectName, ReadAimLine, CallDowncast,
  menus_crafting_rows and the three menus_pending drains (their
  HandleInputEventOf read the vtable without a guard). CallAsCast was
  CallDowncast with a byte offset and is gone: kVtableSlotAsEnemy and
  kVtableSlotAsObstacle are slot indices now (7 and 8). The layout
  constants of minigame_engine.h are in the catalog headers. Left as it
  is: GetDeviceState in engine_keyboard_resync, a DirectInput device and
  not an engine object. The turret files' lines over 100 columns are
  wrapped. In-game check: a swoop
  race (obstacles, pads, the pull toward a pad), the turret game
  (aim line, pull toward a fighter, hit telemetry), crafting on the
  KOTOR 2 workbench, a slider in the options, Esc in the equip picker.
- F6 done as far as it is a duplicate: the physical-key read
  `(GetAsyncKeyState(vk) & 0x8000) != 0` was written out in nine files
  and is acc::hotkeys::KeyDown now. The polls themselves differ on
  purpose: the bring-up and Big Picture threads and the keybind capture
  read raw keys before or outside the hotkey registry, and the pazaak
  wager has its own hold-and-repeat.
- F8 judged, nothing left: the attribute and skill screens already share
  menus_chargen_layout (a PanelDesc per screen). RowPitchForCursorWarp in
  the two files is the one-line call into it, AnnounceValueChange differs
  in substance (the attribute screen tracks modifier and cost), and
  AnnounceChainStepSuffix is one name for four different suffixes.
- F9 done for what was the same: the swoop race and the turret game each
  had the latch on CSWMiniGame written out (enter, re-latch, the two
  truth sources, the exit debounce, LatchedStillValid, five state
  fields). It is minigame_latch now, engine-free with unit tests, and
  TickLatch in minigame_engine reads the engine for it; each Tick opens
  with one switch on its result. One difference in behaviour: the swoop
  race re-latched to any new struct, also one of another kind; it now
  does what the turret did and lets the debounce run. HandleEnter,
  HandleExit and AnnounceEntry themselves differ in substance and stay.
  The turret's HandleEnter reset some forty state fields by hand and
  missed fifteen (stale lead probes, hit times and motion estimates of
  the session before); it assigns a fresh State now, like the swoop
  race, with the two loop sources beside State because they cannot be
  copied. In-game check: a swoop
  race and the turret game start with their opener, and end with the
  race time or "turret game ended" once.
- Bridge: a party/equipment/inventory path; check what K2 needs (all
  but /health, /spoken, /state, /menu, say and key are KOTOR 1 only);
  live check of /objects with creatures and kotor.py kaart --personen
  (needs the game running with the new DLL).
- Static analysis. Until 2026-11-10 kdev build hid the warnings of every
  file that compiled, so "0 warnings" was never checked; it shows them
  now and builds our sources at /W4 /WX (fc88101a; the 32 warnings that
  surfaced are fixed, 76e89f64). The tools, all under tools/analysis
  unless said otherwise:
  - cppcheck.ps1: after every commit, exit 0. Suppressions with their
    reasons in cppcheck_suppressions.txt. With -Style a report; its
    knownConditionTrueFalse findings go with F17 (see there).
  - clang_tidy.py (checks in .clang-tidy): baseline 8, all reviewed
    false positives (audio_loop 261 logs a freed pointer's value,
    combat_log_parse 220, cycle_state 358, engine_area_names 82,
    guidance_autowalk 235 is read in the __except, mod_settings_store 54,
    party_status 79, room_topology_absorb 242).
  - msvc_check.ps1 -Mode analyze: baseline 121. 105 x C28159 (F15), 7 x
    C6385 and 3 x C6054 reviewed false positives, 4 x C6262 (stack frames
    over 16 KB: engine_manager 162, map_ui_cursor 181, menus_chain_input
    125, peek_description_item 313), 2 in the Windows SDK.
    -Mode w4 is the quick warning check without a build.
  - tools/tests/run_unit_tests.ps1 -Mode asan (also in CI) and -Mode
    coverage: 91% of the lines under test; low are engine_compass (61%)
    and engine_keymap_codes (49%).
  - clang-cl -fsyntax-only -Wno-everything -Wunused-* per file, for
    unused code.
  - call_convention.py: every engine function-pointer typedef against
    the engine's own `ret N`, read from an image of each game
    (build/analysis/engine_image_k1.bin from a Steamless copy or kdev
    dump-text, engine_image_k2.bin straight from swkotor2.exe). An
    address that goes through a local or a parameter is followed to the
    constants it can be. Both games: 184 calls fit, 0 mismatches. Not
    checked, 11: 9 virtual calls read from a vtable at run time (all but
    the DirectInput one through ReadVirtual), one member of the chargen
    layout table, one trampoline of our own. Of the
    virtual calls the two fixed slots were checked by hand against all 25
    vtables the sources name, in both games: slot 15 (HandleInputEvent)
    pops 8 on every GUI class, and the As* downcasts (slots 20 to 23) pop
    0 on every control class. On a PANEL those slots are other methods
    that pop 4: CallDowncast must only ever get a control.
    Run it for both games after adding or changing an engine call.
  - engine_twins.py: is the KOTOR 2 address of a Pick(k1, k2) the same
    method as the KOTOR 1 one? Compares the calls, engine globals and
    strings both functions use; a call that agrees counts for the caller
    and the callee. It catches the old popup address. Now: 159 function
    pairs, 110 agree, 0 disagree, 3 mixed, 46 unproven (small accessors
    and the CExoSoundSource setters: nothing to compare). The 3 mixed
    were looked at: 3, 7 and 14 points agree; what KOTOR 1 uses and
    KOTOR 2 does not is the party table in the inventory and abilities
    screens and the screenshot file names in the input handler, which
    fits screens that differ between the games. Two pairs read by hand
    are in engine_twins_reviewed.txt. To prove more of the 46: compare
    immediate constants.
- Done (Anouk, 2026-11-11: Steam only, English only): one executable per
  game. The 2004 relink of KOTOR 1 (Russian, Polish) and the GOG release
  of KOTOR 2 are out: their tables, hook files, kdev sigscan and the
  k2_gog re-scripts (54b3b0b7 and the two commits after it). acc::addr is
  six inline one-liners in engine_addr.h. The header is engine_addr.h now. Left for later: PickGlobal, TodoGlobal and Kotor1Only now do what Pick and R
  do and are kept for what they say at the declaration; Ok() guards that
  only existed for the relink can be judged per module. kdev dump-text
  stays: it makes the KOTOR 1 image for the two engine checks.
  Changelog: the UseObject fix and the KOTOR 2 popup text get their
  lines once they are confirmed in game.
  Run clang_tidy.py FILE on every file a lean rewrites: it caught an
  index of -1 in the first one it was used on (room_topology_doors).
  A check for docs: every `name.cpp` / `name.h` a .md mentions should
  exist in patches/Accessibility (history sections excepted). Done by
  hand on 2026-11-11, together with the names of removed things:
  SECURITY.md described the updater and is rewritten for the fork, the
  developer-probe keys left docs/controls-and-input.md, a few stale
  file names in the llm-docs. Left as history: kotor2-port.md under
  its fork note, audio-internals and persistence-scriptvartable where
  they name a removed probe as removed.
  cppcheck is not in CI: the packages a runner can install are old
  versions and would not give our results.
- F15 judged, no change: GetTickCount and acc::tick::NowMs are two clocks
  on purpose. NowMs is the timestamp of the tick being dispatched, valid
  only inside the fan-out and stale in an input hook; GetTickCount is the
  live clock, and its `now - then` idiom is safe across the wrap that
  C28159 warns about. Replacing one by the other blindly would give
  stale times.
- F16 done: WholeMetres and WholeMetresAtLeastOne (engine_offsets_types.h,
  unit tests in test_geometry) for the 14 places that round a spoken
  distance. The other `+ 0.5f` roundings are degrees, a clock hour, a
  sample rate and a volume.
- F17, raw __try blocks outside engine_reads. `python
  tools/analysis/raw_try.py .` counts them and lists the ones that only
  read or write memory (no call inside): those can go through
  engine_reads. Part 1 done: 307 -> 250. The blocks that return one
  field went by script (ReadPtrOrNull, ReadI32Or0, ReadU32Or0, the new
  ReadU8Or0 / TryReadU8, TryReadVector), some thirty by hand. That also
  removes the cppcheck knownConditionTrueFalse findings (38 left, was
  86): cppcheck does not follow an assignment inside __try, so nearly
  all of them are a variable set in a raw __try and tested after it.
  Left, 53 read-only blocks: the guarded writes (a TryWrite family is
  not there yet: menus_keymap, menus_focus_k2, engine_picker,
  engine_actionbar, camera_orient_turn, camera_spin_guard, pazaak,
  minigame_engine), reads of several fields in one block, array walks,
  and the reads off a hook's stack frame, which stay. The other 197
  call the engine and keep their guard. Seen on the way:
  IsK2UpgradeNoneRow (menus_listbox_workbench) and
  IsUpgradePickerNoneRow (menus_pending_workbench) are one function.
- Max HP of companions: party_status and bridge_state read it with
  ReadCreatureMaxHitPoints (GetMaxHitPoints(1)), which the maker's notes
  in the old combat_query say returns garbage for anyone but the PC.
  ReadClientCreatureHitPoints is the read the character sheet uses.
  Compare Ctrl+H or the bridge with the character sheet in game.
- combat_diag is done (543 -> 263): it only logs now. The idle notice,
  LeaderIsAttacking and the queue readers are in combat_queue; one count
  function, where a round without an action list is 0 (it was -1 and
  silent in OnEngineActionAdded). Not checked against the engine whether
  AddAction can run on such a round; if it can, the announce now says
  "position 1" there.
- Bug, found by Anouk in play (2026-11-10): a locked container that opens
  through a dialog on the object itself is labelled "opens only through
  the story". ReadLockNeed answers StoryOnly for key-required without a
  key name, and gives TryIt only to doors with an OnFailToOpen script.
  Two on Taris: Sealed Strongbox (tar_m03ab; UTP Conversation
  tar03_strongbox, OnUsed k_ptar_boxused) and Security Crate (tar_m03ad;
  tar03_puzzle, k_ptar_puzzlebox). Fix: a placeable with a Conversation
  or an OnUsed script gets TryIt, or a wording of its own; needs the
  CSWSPlaceable offsets of those two fields, verified. Also check that
  Enter next to such a container starts the dialog (her attempts were
  from 15 m and the walk was cancelled). Seen 2026-11-11: Enter at 1 m
  starts the crate's dialog, and the six holograms are in the , / .
  cycle as containers.
- Three findings from the Security Crate puzzle (2026-11-11, installed
  build d875083, log patch-20261110-224439.log):
  (1) A hologram that is switched on is locked by its script and then
  reads "locked, Security 18, best in your party: Carth with 2". Anouk
  took that for a lock to pick. A placeable without an inventory has no
  lock to pick: no Security line there (needs the has-inventory field).
  (2) The leader stood boxed in between the crate, the holograms and
  the follower. Enter on four objects 3 m away queued the use (outer
  queue 0 -> 5, never draining), "Approach: stalled but target position
  unresolvable - disarm quietly", and nothing was spoken. Tab to the
  follower on the other side solved it. Cause: see (3).
  (3) Crash: the bridge's /do use (kotor.py pak) on hologram 0x68:
  c0000005 in accessibility.dll right after "UseObject dispatch".
  Cause found in the dump and fixed in 4c91ad0d: UseObject pushed two
  arguments to KOTOR 1's AddUseObjectAction, which pops one, so its
  caller's EDI/ESI/EBX were garbage after every use. That is also why
  (2) could not read the target: "Approach: armed ... targetObj=
  00000068" is the handle where the object pointer should be. After
  the fix, check in game: Enter on something out of reach should now
  say that the way is blocked; if it still says nothing, (2) stays.
  The installed build d875083 still has the bug: no kotor.py pak there.
- Tab: on a leader swap both party_leader_announce's Tab window and
  TickLeaderChange speak the new name. Check in game whether it is heard
  twice; if so the Tab path only needs the no-swap case.
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
  the layout), and the false positives
  (bridge_describe containerOutOfBounds, cycle_state and skillinfo
  uninitvar).
- BTN_WAEHLEN / BTN_ABBRECHEN stay in a comment: they are .gui control
  tags, not prose.
