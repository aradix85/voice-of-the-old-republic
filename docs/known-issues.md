# Known Issues

Status tracker for this fork, in five buckets. Most entries come from the original mod's testers and were carried over.

- **Bugs** — regressions or broken behaviour to fix.
- **Unreproduced** — reported issues we can't reliably reproduce yet; need a repro before fixing.
- **Planned** — future feature work that's not currently in flight.
- **Monitor** — shipped features whose behaviour we're still watching in live play.
- **Polish** — quality-of-life refinements; the feature works but has rough edges.

When an entry is closed, move it out of this file (the corresponding fix or commit message is the durable record).

## Bugs

### A container that opens through its own dialog is labelled "opens only through the story" (found in play 2026-11-10)

`ReadLockNeed` answers StoryOnly for key-required without a key name, and gives TryIt only to doors with an OnFailToOpen script. Two on Taris: Sealed Strongbox (tar_m03ab; Conversation tar03_strongbox, OnUsed k_ptar_boxused) and Security Crate (tar_m03ad; tar03_puzzle, k_ptar_puzzlebox). Fix: a placeable with a Conversation or an OnUsed script gets TryIt, or a wording of its own; this needs the CSWSPlaceable offsets of those two fields, verified. Enter from 1 m does start the crate's dialog.

### A placeable without an inventory reads as a lock to pick (2026-11-11)

A hologram that is switched on is locked by its script and then reads "locked, Security 18, best in your party: Carth with 2". There is nothing to pick: no Security line for a placeable without an inventory (needs the has-inventory field).

### "Locked, may open when you try" is too wide

Every door with an OnFailToOpen script gets it, also when the script only barks a line (Arena Door, cp_tar02_duelbrk). Idea: read the script, and treat one that only does BarkString / SpeakString as no story door.

### Class selection: Left / Right pick a class once Up / Down was used (2026-11-11, in the original mod too)

Before the chain is bound the engine moves the selection itself. After one Up or Down the chain owns the panel, and `HandleLeftRight` (menus_chain_input_keys.cpp) treats Left / Right as a cycle arrow: it activates the neighbour, which `classIconOffsetX` makes the class two places on. On this panel Left / Right should step the chain.

### A run of panel texts is spoken on the load into a new game (KOTOR 1)

"Add Power", "Buy", "You cannot equip this item", "control 8", "Party Inventory", "Script Selection", "Active Quests", "Options", "Load a saved game", "Messages", "Load Game", all in the second the panels are built, before "Area: ...". KOTOR 2 had the same and was silenced.

### KOTOR 2 crash entering the Peragus Fuel Depot — third-party item, engine null-deref (diagnosed 2026-08-24, no code change yet)

Kenny's game died on the area transition into the Fuel Depot
(`userlogs/kennycrashfuel0782208.7z`, `k2-patch-20260821-231904.log` ends
23:40:00, dump `k2-swkotor2.exe.17344.dmp`). Read of `[NULL+0x70]` on the main
thread, inside the engine's item loader — decompile of the K2 fault site
(`FUN_006014f0`, called from the GFF item loader `FUN_00601740`, kotor2 Ghidra
project) shows it calling `GetBaseItem(item+0xc)` and dereferencing the row
without a null check (it null-checks the row's *field* at +0x70, one
instruction too late).

The dump still holds the GFF being loaded: `heavymininglaser.uti`, tag
`HeavyMiningLaser`, `BaseItem=106`. Every baseitems.2da we ship stops at row
105 (verified against the local install with the same source pins), and no
bundled mod carries that template — it is a third-party mod on Kenny's machine
(likely a Steam Workshop subscription or a mod installed outside our
pipeline) whose accompanying baseitems.2da row never made it into the
effective Override. Recurs on every entry to that module — potentially
progress-blocking for Kenny.

Follow-ups to decide:
- Ask Kenny for their mod list / Workshop subscriptions; the mod that ships
  `heavymininglaser` needs its baseitems.2da row restored (reinstall after our
  bundle) or the mod removed.
- Check whether our bundled-mods install can clobber a pre-existing modded
  baseitems.2da (install-order conflict) — if yes, this class of crash is
  partly ours to prevent.
- Defensive patch SHIPPED 2026-08-25 (both games, fallback
  row 0), live-tested in BOTH games the same day: CLI-injected
  CreateItemOnObject of a BaseItem=200 item survived, item usable in
  inventory, `BaseItemGuard` log lines present (K1: id=200/92 rows,
  ~28k dedup-collapsed wrapper hits; K2: id=200/106 rows — the same
  106-row table Kenny's id=106 item overruns). What remains open here is
  only Kenny-side confirmation that the Fuel Depot now loads. Target
  confirmed by decompile in both games: the baseitems-ARRAY accessor
  `CSWBaseItemArray::GetBaseItem(int id)` — K1 `0x005b31a0` (Lane's DB names
  it; identical shape to K2's `0x006d5200`: NULL for table-not-loaded / id<0
  / id>=count, count at this+0, array at this+4, 200-byte stride). Hooks
  land AFTER the engine's table-loaded check, so the startup-probe NULL
  stays untouched; out-of-range ids clamp to row 0 and log under the
  `BaseItemGuard` tag. Handlers in `baseitem_guard.cpp`.
- First live test (CLI-injected CreateItemOnObject of a BaseItem=200 .uti,
  2026-08-25) CAUGHT A HOLE: K1 still crashed in CSWSItem::LoadDataFromGff
  — its `CALL CSWItem::GetBaseItem` returned NULL despite the accessor hook
  firing, because K1's optimised build INLINED the accessor into that
  wrapper (`0x005b4790`). Fixed with a second K1 hook on the wrapper's
  inlined bounds check (`0x005b47a3`), same handler. An IMUL-by-200 sweep
  over K1's whole .text found exactly three sites (accessor, wrapper copy,
  loader's count*200 allocation) — coverage now complete. K2's unoptimised
  wrapper CALLs the accessor (byte-verified) — one hook suffices there.
- Test tooling (all in place): `Override\acc_test_baditem.uti` (BaseItem=200)
  + `Override\acc_givebaditem.ncs` in both games,
  `build\runscript\runscript.exe <script>` injects the engine's runscript
  cheat via KotorMessageInjector (no console needed — the console key is
  broken on non-US layouts anyway). Verification: run
  `runscript.exe acc_givebaditem` with a save loaded → no crash, item in
  inventory, `BaseItemGuard` line in the patch log. On Kenny's save the
  Fuel Depot should load with the id=106 line. Remove the two test files
  from both Overrides before release.

### Exit direction WORDS are measured from the junction's map point, not from where the player stands — found 2026-10-28, revisit later

Surfaced by the (since removed) 3D exit descriptions: the spoken word can be
wrong for where the player stands.

**Known (log patch-20261028-125607.log, Taris Upper City, KOTOR 1):**

- A Kreuzung there is a SINGLE nav node at (131.6, 93.1), north of the street.
  The label is built once per cluster from that node (room_topology
  ClassifyCluster): "Kreuzung, Nord, Einfache Sicherheitstür Süd-Ost, West".
- The announcement fires on cluster change, when that node becomes the nearest
  labelled one — the player was at (125.7, 85.9) / (124.7, 89.1), ~9 m away,
  walking along the south side of the street.
- The security door at (134.3, 90.7) is south-east of the node, but
  east-north-east of the player (~30° north of east). The user heard it from
  the north, next to the "Nord" exit, and reported the word as wrong. Same in
  the neighbouring junction (node (112.8, 85.7), player ~7 m west and 4 m
  north of it).
- Every exit word in every region label has this frame: octant from the
  cluster centroid / node, independent of where the player is when it is
  spoken. Small rooms hide it; wide streets and single-node junctions expose it.

**Suspected:** worst in wide streets and open outdoor areas (Kashyyyk), where
the player passes a cluster's node at a distance; not yet checked there.

**Decided (with the user, 2026-10-28): the centre rule.**

- Labels stay per-cluster constants. Words measured from the player were
  rejected: "the game can be confusing enough without labels dynamically
  changing". Naming the frame in speech was rejected as noise.
- Instead the TIMING moves: a cluster whose exit words are measured from its
  centre speaks when the player is near that centre — within a quarter of
  the cluster's extent — or has stopped getting closer to it (passing its
  closest point, or standing still). Left the cluster before that: silent,
  as the dwell gate already did for pass-through.
- Covers Kreuzung, merged rooms (Bereich) and the zone-scale / open Bereich
  under 40 m. It replaces the 500 ms dwell for those and the 1 s Platz delay
  below 40 m.
- Unchanged: Großer Bereich (>= 40 m; the player rarely nears its centre,
  so it keeps today's timing), corridors and dead ends (keep the 500 ms
  brush-past dwell: an axis word is true anywhere in a corridor, and a dead
  end is most useful heard early), stability ticks and the flap guard.
- Known limit: a single-node crossing passed at a distance (the Taris street
  case above) still speaks off-frame — the closest approach is still ~7 m
  away. That is a clustering issue, not timing; revisit only if it recurs.
- Built 2026-10-28 (transitions.cpp `CentreRuleReady`). First test (K1,
  patch-20261028-180057.log, Taris Südliche Apartments): words fit much
  better, but the player overshot even medium crossings. Every crossing
  there is a single node (extent 0, so "near" never fired) and the stall
  test fires only after the closest point: 300 ms + the noun + reaction at
  ~4.5 m/s = 4-5 m past, out into the next corridor (clusters 14, 29, 30).
- Second build, same day, tested in K1 the same evening ("that felt now very
  good"; committed 0de0928):
  - "Near" uses the floor footprint: the larger of node spread and the
    narrowest wall-to-wall axis of the clearance probe (those halls: ~11 m,
    radius 2.8 m).
  - Lead: a moving player is announced once the closest point of their
    current path is less than (measured noun length + 500 ms reaction)
    ahead, or already behind. The stall test stays for standing still.
  - Log: `centre rule met: lead (... closest in Xs at Ym ...)`.
- Follow-ups from that test's log (patch-20261028-182126.log): the spawn
  after an area load read as 26.9 m/s (speeds above 15 m/s now ignored), and
  the noun length was once taken from a corridor list's first exit word
  ("Nord"; now only an unplaced first piece followed by exits counts).

## Unreproduced

### Dead keyboard on a machine with no mouse — fix built, awaiting a no-mouse session

kenny's KOTOR 2 0.7.5 session (`userlogs/kenny075keyboarddeath.7z`) reached the main
menu with speech and cursor working and never delivered one keystroke to the engine.
Root cause is the engine's own DirectInput teardown: any failure creating the mouse
device releases the keyboard device too, and nothing rebuilds it — see
`docs/llm-docs/gui-and-input-internals.md` → `directinput_device_lifecycle`.

The fix (mouse-teardown block + per-frame retry latch + a DirectInput state line in
the log) is in and verified on the HEALTHY path in both games: the block installs
(5 call sites on KOTOR 2, 2 on KOTOR 1) and the state probe reports the interface,
keyboard and mouse correctly through startup and focus loss.

What is NOT verified is the failure path itself, because this dev machine has a
mouse and DirectInput creates the device normally. Unplugging a USB mouse may not
reproduce it either — the failing step could be any of CreateDevice, SetDataFormat,
SetCooperativeLevel, SetProperty or Acquire, and the handler now names which one in
the log. Confirmation has to come from kenny's next session: expect either a
`MouseTeardownBlock: mouse bring-up failed at <step>` line with a working keyboard,
or no such line at all (in which case the teardown comes from somewhere else and the
`DirectInput state` lines will show where).

## Planned

### Nameable personal map pins

Let players give their own map pins a name when they drop one, and read that name back when cycling to the pin. Personal pins currently announce only a generic label plus position, so a player who marks several spots can't tell them apart. Needs a text-entry path that works from the keyboard with a screen reader (the editbox handling shipped for chargen + save naming — `menus_editbox.cpp` — is the closest existing mechanism, though map pins aren't an engine editbox so the entry surface differs), storage of the label alongside the pin in the save, and the cycle/focus announcements updated to speak it. See `map_user_markers.cpp`.

### Bundle the resolution / widescreen patch — once we are out of beta

Offer the community resolution / widescreen + HR-menus patches as an installer option, and make our own code robust to the layout changes they introduce. Deliberately deferred until the mod leaves beta: it widens the support surface (several of our paths still assume a particular screen geometry — screen-pixel coordinates, the OS cursor, hardcoded hit-test offset compensations), and a beta is the wrong time to add a second variable to every bug report. Prerequisite when we do take it on: audit every place we depend on screen geometry and read it live from the engine instead of assuming. See `archiev/installer-design-original.md` (Widescreen / HR-menus bundling).

## Monitor

### To check in play after the 2026-11 restructuring

- Tab: the leader's name had two announcers and was heard twice, or as the old name and then the new one (confirmed by Anouk, 2026-11-11). party_leader_announce has one `Tick` now; confirm in play that the name comes once, and that Tab alone in the party still answers.
- Max HP of companions: party_status and bridge_state read it with `ReadCreatureMaxHitPoints` (GetMaxHitPoints(1)), which the original notes say returns garbage for anyone but the player character. `ReadClientCreatureHitPoints` is what the character sheet uses. Compare Ctrl+H with the character sheet.
- Enter on something out of reach: the use call left KOTOR 1's caller with garbage registers until 4c91ad0d, so the stalled approach could not read its target and gave up without a word. It should now say that the way is blocked. In Anouk's logs of 2026-11-10 and 11 on the old build, 212 of 610 armed approaches carried the handle where the object pointer belongs, and 63 ended in "target position unresolvable - disarm quietly". She reports (2026-11-11) that after "way blocked" she often cannot walk on until she reloads, and that turning the character does not always free it. Whether that is gone with the fix is open: when it happens again, look at the live state (action queue depth, `Approach:` and `Autowalk:` lines) before she reloads.
- KOTOR 2 tutorial popups: the popup called the wrong engine function for its text until 9b6fcb1b (a vtable slot off by one). Its text should show and be spoken now; no KOTOR 2 session has confirmed it.
- combat_queue counts a round without an action list as 0 where it was -1 and silent; if the engine can add an action on such a round, the announce now says "position 1" there.
- NVDA's "speech interrupt for typed characters" (Keyboard settings) cuts a line while a key is held (map cursor, turret fire key). The Windows voices used to dodge that; they are gone.

### KOTOR 2 party selection read "control 13..24" — v0.8.3 fix FAILED in the field, re-fixed 2026-10-24, awaiting test (GitHub issues 6, 7 and 9)

**v0.9.0 field log (2026-10-29, `k2-patch-20261029-192616.log`, Telos Czerka
site): names now correct, but the screen was not operable — fixed 2026-10-30,
built, untested.** Every portrait read "<name>, in party/available" as
intended. But Up/Down never reached them: KOTOR 2 opens the party screen
from strip slot 4 (its SwitchToSWInGameGui maps id 4 to the PartySelection
slot), so the strip stays in front, and the sub-screen table in
`menus_monitors.cpp` had no PartySelection row, so the drill never armed.
Arrows walked the strip, and Enter on its party-order controls popped "Party
order can not be changed in the Party Select screen". Only the engine's
Right arrow moved through portraits, and it spoke "control 15" on an empty
slot. Fixes: KOTOR 2-only PartySelection row (strref 48625); portraits say
"in party, locked" / "unavailable" when GetNPCSelectability is off (that
roster had Bao-Dur locked in, T3-M4 locked out); empty slots say "Empty
slot"; "Party full" no longer speaks over a locked companion. The strip's slot-4
label read "Messages" (strref 1563) on KOTOR 2; it now uses 48625 "Party
Selection" (menus_extract.cpp icon table). Test: the fifth strip button
says "Party Selection"; open with the hotkey and with Tab, Up/Down step portraits, Enter
toggles Kreia/Atton, OK commits, Esc closes; KOTOR 1 party screen unchanged.

**v0.8.3 did not fix it (issue 9, ricki35, 2026-09-23, mod 0.8.3, Steam).**
The tester's `k2-patch-20260923-125859.log` still speaks "control 13" / "control
15" on two visits, and its panel walk prints every BTN_NPC portrait as
`vtable=009A6D9C`. The 2026-09-17 analysis below had it backwards: `0x009A6D9C`
WAS correct. The button ctor `0x0089CDA0` stores `0x009A40FC` (the
`CSWGuiRadioButton` base, shared by nine constructors) and then immediately
overwrites it with `0x009A6D9C`; RTTI names the latter
`CSWGuiPartySelectionButton` (`docs/llm-docs/re/k2/k2-vtables.csv`). The fix
read the first store. Verified on the clean Steam reference. Constant restored and built; not yet tested in game. Side effect of the
miss: the unrecognised top-middle portrait fell through to the sibling-label
reader and spoke "2 Available Slots"; a recognised portrait stops the ladder
before that step. The rest of the 2026-09-17 work (offsets, 12-slot roster,
names) has therefore still never run live — the check list below is still the
test.

Reported by ricki35 (issue 6 on 2026-09-09, issue 7 on 2026-09-16, the same bug).
Nobody on our side had reached the K2 party selection screen before. Root cause,
from K2 decompiles (the attached patch logs were both KOTOR 1 sessions): the K2
`kVtableCSWGuiPartySelectionButton` constant was never witnessed and matched
nothing, so no portrait was recognised and every one fell through to the
"control N" placeholder. The K2 portrait control also sits 4 bytes into its
data element, so every K2 member offset was off by 4 and the roster index had
never been ported. Fixed via one shared reader (`ReadPartyPortrait` /
`PartySelectionChosenCount`), a 12-slot roster, the K2 GetNPCSelectability twin,
and BTN_ACCEPT dropped from the chain on K2 as on K1. K2 names come from the
engine (GetNPCObject template-loads benched companions); the K1 fixed name table
is untouched. Engine model: `docs/llm-docs/ingame-screens-reference.md`,
partyselection K2 section.

What to check in the next K2 round: on the party screen, arrowing speaks
"<name>, in party" or "<name>, available" for recruited companions only; Enter
toggles; a third add speaks "Party full"; OK commits the chosen party. The
`Menus.PartyPortrait` trace lines carry slot, selected, flags, partyId and
availability for every portrait read. KOTOR 1 needs one regression pass over the
same screen (the reader is shared code).

Two side findings from the same report:

- **The attached K2 crash dump (`swkotor2.exe.3164.dmp`, 2026-09-15, mod 0.8.1)
  is unrelated to the party screen.** Null dereference in the engine's
  dialogue-camera update: the object lookup for the conversation's camera or
  speaker returns null and the result is called through. No mod frame is on the
  stack. The session's K2 patch log was not attached, so a mod contribution is
  neither shown nor excluded. Ask for it if the crash recurs.
- **The tester's log bundle held only KOTOR 1 logs because their installer was
  older than v0.7.2.** Its system-info.txt has the pre-v0.7.2 one-game format
  ("Game: KOTOR 1", the `swkotor*.dmp` pattern that also caught K2 dumps).
  Two-game collection shipped in v0.7.2; the current collector is correct.

### Lost key releases (stuck turn / walk) — engine-level fix built 2026-09-14, KOTOR 2 confirmed the same day, KOTOR 1 untested

The KOTOR 2 uncommanded-camera-spin report of 2026-09-14 (`patch-20260914-085546.log`,
a clockwise spin from 09:10:58 to the quit at 09:12:24, cursor NOT in the edge
band) was a stranded turn-key release. Decompiled on both games the same day:
the engine reads the keyboard as buffered DirectInput transitions and never
re-reads the device, so a release discarded by any of its own gaps stays "down"
until the key is pressed again. The gaps: every raw-input SetActive 0→1 edge
(dialogue start/end, pause and quit popups, area transitions, load screens,
movies, window focus) and the reader's own "input lost / not acquired" path,
which answers an empty batch. The pause key's own release is lost on
essentially every press in the KOTOR 2 logs (`Diag.ClientHIE` 218 down, never
up) — harmless there, proof of the mechanism. Input-class switches also drain
the buffer but zero every key state first, so they release rather than strand.

Fix: `engine_keyboard_resync.cpp` wraps the one call through which the per-frame
event fetch pulls the keyboard batch, diffs the engine's view against
`GetDeviceState`, and appends a release for any key the engine still holds that
DirectInput reports up, after two consecutive frames of disagreement with no
transition for that key. Presses are never synthesised. Full model in
`docs/llm-docs/gui-and-input-internals.md` "keyboard_transition_stream".

What to look for in the next logs of either game: `KeyboardResync: installed`
at bring-up; a `synthesised release for DIK 0x..` line whenever a loss was
repaired (0x2e = C, the user's turn-right key); `KeyboardResync.Device` edges
dating any DirectInput-level gaps. A `CameraSpinDiag: UNCOMMANDED ROTATION`
that is NOT followed within a second by a synthesised release is a new cause.
KOTOR 2 confirmation (`patch-20260914-115230.log`, 12 minutes of deliberate
provocation, no spin, no fault): four releases repaired — Enter at a load end,
F4 at the quit popup twice, W during a popup — every one at an engine
`active=0 → 1` edge, none anywhere else, i.e. no misfire in ordinary play.
The `KeyboardResync.Device` edge only reports a device lost while the
engine still believes input is active; gaps the engine itself opens are
already dated by the `DirectInput state` poll. KOTOR 1 shares the code and
its call site is byte-verified offline, but has not had a session yet.

Which of the two invisible gaps produced the 09:10:58 episode is still
unknown; the new lines answer that on the next occurrence.

**Second KOTOR 2 spin, 2026-09-14 (`patch-20260914-122504.log`, 12:30:12 to the
quit at 12:30:25): NOT a lost release.** The re-sync ran every frame through it
and found the engine's key view and DirectInput in agreement; its four repairs
that session all sat at real `active=0 → 1` edges, the last one at the quit
popup. No pad, no snap-turn of ours, no UI owning the camera, the player
standing still at onset, cursor parked at (24,2) and never moving. The KOTOR 2
`UpdateCamera` twin was decompiled the same day (camera-and-swoop.md, "KOTOR 2
twin"): its band is 2 px, so x=24 is outside it; the vertical band the park does
sit in is a no-op in the follow camera; the native right-stick source needs a
controller enumerated at start (there were none); the mouse-delta path needs
cursor motion. Every input the camera itself reads is therefore excluded by the
log, which leaves the player creature rotating with the chase camera following
(indistinguishable from a camera turn in the spin report), or a source outside
`UpdateCamera`. KOTOR 2 snaps the player's facing to the selected target — the
instantaneous compass jumps in that log coincide one-to-one with target changes
(E cycling, show-object changes under the parked cursor), and the engine target
changed three times during the spin (the `Combat.Diag: DELTA target` lines
date those). Instrumentation built and applied to both games 2026-09-14:
every `UNCOMMANDED ROTATION` line now also carries `playerYaw` / `playerRate` /
`playerTurn` (the player creature's own facing, from the server object — a
rate matching the camera's means the creature turned and the chase camera
followed; near zero means the camera's own turn path), `turnAxis` (the
engine's PollInput(0x11c) answer, i.e. its own held-turn-key view),
`stickX` (K2 native right stick, `n/a` on K1), `mouseY`, `rightClick`
(K2 global now known) and, K2 only, `acclMirror` / `turnedFlag` (module +0x98
/ +0x100, written only when AcclTurnCamera applies a turn). The END line
carries `playerTurn` too. Read the next spin's first report line: it names
the driver.

### World hotkeys that still act while a screen owns the keyboard

Audit run 2026-08-20 alongside the galaxy-map fix above, over every Win32-polled
hotkey that can reach an engine dispatch. Three surfaces remain gated on
"is there a world" alone (`GetPlayerPosition`), with no UI-block check:

- **Shift+H combat queue** — arms over any panel, and its rows call
  `RemoveLastAction` on the live combat round. Worst case is mutating a round
  from inside a screen; it cannot injure a module load. One-line fix if wanted:
  `ForegroundPanelBlocks()` at the opener, same call the action menu uses.
- **Camera orient (N, turn to exits or a cardinal)** — synthesises the
  player's bound turn key via `SendInput`, so with a screen up those scancodes
  land in the screen. Injects no action, but the mod's own input detectors see
  the injected keys too.
- **Shift+L level-up** — opens the engine's level-up wizard over whatever is
  foreground. Has its own already-open and can-level-up guards, so it stacks a
  panel at worst.

None is reproduced in a log; all three are the same *shape* as the galaxy-map
fault (one press, two consumers, one of them the world) and each would cost a
test round to change, so they are listed rather than fixed. The detector for
the rest is now in the log: `Interact: <key> GATE GAP?` fires whenever an
in-world dispatch runs over a foreground panel whose kind `WorldInputOver`
calls Unlisted (it neither blocks nor is one of the free kinds: HUD, pause,
bark, fade, Unknown). Grep tester bundles for `GATE GAP` before adding kinds
by guesswork. The AreaTransition banner is Unlisted on purpose, so a press
at an area transition logs one such line.

### Grass crash — worked around with `Grass=0`, root cause not fully pinned

Shipped in 0.7.7: the installer sets `Grass=0` in `[Graphics Options]` on both games, which removes the faulting code path. Full analysis in `docs/grass-crash-analysis.md`.

What is confirmed: the fault is an access violation inside the Intel OpenGL driver, called from `GLRender::DrawLightmappedGrass`; grass is always submitted as client-side vertex arrays; and `DrawLightmappedGrass` derives its attribute base as `param_1 + param_6 * 0x30`, which is only correct when `param_6` is the bin's *total* quad count — so the per-sub-bin draw loop in `RenderGrassPolys` feeds the driver attribute pointers aliasing into the position block.

What is **not** explained: that defect reads wrong data but stays inside the allocation for every stride the engine uses, so it does not by itself produce the page fault. Most likely remaining candidate is a sub-bin count list disagreeing with `field6_0xc`. `CAurTriangleBin` is a Ghidra PlaceHolder struct, so per-field reasoning is unreliable until it is re-typed.

Watch for: any grass-area crash report from a user who has re-run the 0.7.7 installer (would mean `Grass=0` does not gate the path as assumed — we traced the option to the `enableGrass` global but never traced which caller reads it).

### Tick-clock handling is inconsistent across the codebase; a mass change may be needed

A tick is one rendered **frame** — `core_tick` fans out from a detour on `CSWGuiManager::Update`, called once per frame from the engine main loop — so the interval between ticks is whatever the framerate is: ~16.7 ms at 60 Hz vsync, ~6.9 ms at 144 Hz, less with vsync off. There is no fixed tact to assume.

Until 0.7.3 nothing published a tick timestamp. `core_tick` was built as a dispatcher (fan-out ordering) and its `QueryPerformanceCounter` reads were private to the watchdog's SLOW FRAME / SLOW TICK diagnostics, so all 132 clock reads across 42 files call `GetTickCount` independently, at ~15.6 ms resolution. `acc::tick::NowMs()` now publishes the tick-start stamp (same QPC read the watchdog already takes, so it costs nothing extra), but only `camera_announce` consumes it.

**Why this was not caught by any duplication sweep:** there is no duplicated helper to find. A `GetTickCount()` call looks identical and idiomatic at every site; what distinguishes a safe use from an unsafe one is what the caller does with the subtraction two lines later, which no naming or structure pass inspects.

Sorted by what the clock is used for:

- **Deadlines** ("have N ms passed") — the large majority, and correct as-is: one quantum of error against a window of hundreds of ms. No migration needed.
- **Integration** (`speed × dt` — `map_ui_cursor` cursor stepping) — also correct, because it updates its last-stamp unconditionally, so a zero-length tick contributes nothing and the next tick's dt covers the gap; the dts still sum to real elapsed time.
- **Differentiation** (`rate = delta / dt`) — the only unsafe pattern, because a near-zero interval does not degrade a rate, it falsifies it. Three sites: `camera_announce` (fixed in 0.7.3 — it reported 0°/s mid-turn and fired three spurious stop cues in one second), `audio_footstep_suppress` (divides over a ≥500 ms window, ~3% error, fine), and `camera_spin_guard` (per-tick yaw delta; a `dt > 0` guard prevents a divide-by-zero but a zero-length tick reports 0°/s, so the edge-spin guard can skip a correction tick — shipped behaviour, self-correcting next frame, not yet fixed).

**Open question:** whether to migrate more broadly or leave the deadline sites alone. Current judgement is to leave them — they are correct, and a mass change would be a lot of untested behaviour for no benefit. Revisit if a third differentiation site appears, or if a high-refresh-rate tester reports timing-dependent oddities.

**Constraint on any migration:** `NowMs()` is the *current tick's* timestamp, not a live clock. It is only valid inside the Dispatch fan-out; code reached from an input hook, a menu callback or a hotkey runs outside the tick and would read a stale value.

### KOTOR 2 does not refuse impossible actions — we announce their absence instead

A medkit used on a full-health character behaves differently in the two engines, and only one of them tells the player anything.

KOTOR 1 refuses it at the GUI layer: `DoPersonalAction` finds the entry's usable bit clear, plays its error sound, and never dispatches. The entry's flag word carries a reason code, so we speak the engine's own sentence — logged as `ActionBar: variant flags ... usable=0 reason=5` followed by `refused — speaking engine reason strref=0xa602 [Volle Gesundheit]`.

KOTOR 2 mostly cannot: the answer to this entry's original open question (decompiled 2026-08-12, the dead-Medicine-row investigation) is that K2's item appender **never initialises the flag word for inventory items** — only the three equipped-slot entries get one. What we read there is heap garbage (ASCII string fragments in the logs), the engine gates on the same garbage, and `VariantRefusal` therefore skips the refusal verdict for K2 item entries entirely (a garbage bit would fake a refusal with a random reason). Force/feat entries DO compute real flags on K2 — "Macht erschöpft" (`usable=0 reason=1`) was observed live — so the mechanism exists, it is just never fed for items. The no-op watch stays as the honest answer for silent item refusals.

**The K2 Medicine row needed a deeper fix (2026-08-12).** K2 gives every medical-category item a TWO-STEP use flow: the press arms a target-pick (MainInterface `+0x15230` armed / `+0x15234` party slot) that only a mouse click on a party portrait completes — and `DoPersonalAction`'s preamble wipes the state before the medical handler reads it, so from the keyboard the row can never fire at all (one press wedges it armed-with-no-slot; the 2026-08-11 21:58 "flip" in this playthrough was simply the first medical press whose base-item row byte routes into the two-step flow). The shipped answer bypasses that machinery: `SendUseItemRequestK2` emits the same client→server use-item message the portrait click's consume path emits, and the unified menu adds a party-member picker (narrated member preselected, Shift+Enter = instant self, bare 5 = instant self). The `Diag.UseItemReq` hook on the message writer doubles as the no-op watch's cancel signal: a sent request proves the press acted, which also fixed mines announcing "Nicht möglich" while visibly planting (they act without a combat-round add).

**One false-positive class found and fixed (2026-08-08).** The watch was armed for every menu fire, and target-row actions tripped it three for three on a footlocker: Sicherheit, Sicherheits-Überbrücker, and a Schallmine that visibly blew the lock open all announced "Nicht möglich" while working perfectly (`patch-20260808-103232.log`). Target rows act without a combat-round add as a matter of course, so the watch is now armed only for the personal block — `ArmNoOpWatch`, called from the menu's personal path and from bare 4..7, never from `ArmUserQueueAdd` itself.

**Still watching:** KOTOR 2's Combat Behaviour column (key 8) may legitimately queue nothing — if it does it will wrongly announce "Nicht möglich" and needs the same send-witness treatment. And a server-side drop of a direct-sent medical item (e.g. full health) is silent-then-generic rather than reasoned — the engine's client-side CanUseItem gate (`0x005D2F80`) is skipped by the direct send and could later supply real reasons.

Code: `engine_actionbar.cpp` (`VariantRefusal`, `SendUseItemRequestK2`, the always-on flags log), `combat_queue.cpp` (`ArmUserQueueAdd` / `TickNoOpWatch`), `combat_diag.cpp` (`OnUseItemRequestK2`), `unified_action_menu.cpp` (`SpeakRefusal`, the medical party picker).

## Polish

### Starting an autowalk while one is active should switch, not cancel

If an autowalk is already running and the player starts a new one on a different target, the current behaviour just cancels the existing one. Instead it should immediately start the new walk — switching the route in one gesture rather than requiring cancel-then-start.

### Mod-settings sliders only save after pressing Enter

The sliders under Mod settings (e.g. hint-sound volume) only persist their new value to `acc_settings.ini` once the user presses Enter on the row. Adjusting a slider with Left / Right changes it for the session and previews the new level, but the change isn't written until an explicit Enter — so a player who tweaks a slider and leaves the menu without pressing Enter loses the change on next launch. Slider adjustments should persist on each Left / Right step (the same way the value already updates live), without needing a confirming keypress.
