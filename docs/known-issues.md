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

### Found in play 11 to 20 Nov 2026: doors and transitions

- A closed lift door the player stands right in front of is counted on the wrong side (18 Nov 2026, Leviathan hangar lev_m40ac). The player stood at (18.0,154.9) inside the lift cabin, the door "Elevator" at (19,155) was closed; the mod said "East, to a junction, new; Elevator West" for the place and "leads West, to a dead end" on the door. Every walk to the east then gave "Movement cancelled, way blocked" (log "route walk: over at hop 1/13 (the hop keeps stalling)", "explore walk: stop after 0 hop(s), goal 10 (blocked)"), and the mod neither named nor opened the closed door. Workaround: select the door "Elevator" and press Enter (done at the time with the bridge's kotor.py gebruik command). Suspected: the same place-in-front-of-a-door rule as 1b43996f, now with the margin the other way. To investigate with log patch-20261118-161858.log around 20:00Z.
- Yavin Station (liv_m99aa): the mod does not name the doors as doors and plans routes straight through them ("Suvam Tan, via North, about 105 metres to walk"). All doors there are Static and Plot and open only through the Access Panel beside them (OnUsed yav50_panel02 to 04 and yav50_panel). Order from the airlock at (7,-3): Access Panel 6 at (9,-2) opens Yavin Door 2 at (7,-5), the exit to the Ebon Hawk (LinkedToFlags 2, not in the list of transitions; "No transitions in range"); Access Panel 5 at (9,19) opens the door at (7,20); Access Panel 4 at (9,31) the one at (7,32); Access Panel 3 at (9,68) the one at (7,70) and starts the scene on the first visit. The map note "To Ebon Hawk" lies on the spot where the player arrives: Enter on it does nothing (18 Nov 2026, "Use To Ebon Hawk", "settled within reach, gap 0.00m"). Enter on a panel further away stalls against the closed door before it ("Movement cancelled, way blocked. Access Panel"), and at the panel the player stands beside the floor: a long walk from there does not start (WalkTo dispatch without movement). Workaround: Enter on the panel, then walk through the door (the bridge's kotor.py loop 7 -7 command for the ship). Done for the player on 18 Nov 2026, 23:00: the bridge's kotor.py pak command on the panel, if needed kotor.py gebruik as well, then kotor.py loop in stretches of 10 metres along x 7.2 (29.5, 36, 46, 56, 64). Proposed fix: treat a Static door with a transition, or with a panel that opens it, as a door, and pass the map note on to that transition. Investigated on 20 Nov 2026 and deliberately not built: in the whole game 74 doors are Static only (cosmetic), 30 are Static and Plot, 41 are Static and locked (with or without Plot) and 1 is Static, Plot and has a transition (the Yavin exit); counted with a separate PyKotor script over the UTD of every door in every module. Static and Plot are presumably the doors a script opens: the six on Yavin, but also four in sta_m45aa and eight in unk_m44, areas the current playthrough has still to reach. The one place that decides what is cosmetic is `IsDoorCosmetic` (engine_area_doors.cpp), with twenty callers in the region layout, the numbering and the routes. Changing it without seeing it in the game can break routes in those last areas (a cosmetic door in a niche then becomes a barrier). First look in the game at what the bridge's /objects says about such a door (open, locked) before and after the panel, and only then build: treat a Static door with Plot that is closed as locked through the story, and as a door once it is open.
- Enter on an open door with the transition right behind it only walks to the door (18 Nov 2026, Sith base manm27aa: "Elevator to Ahto City, open" at (101.5,35.6), the exit "Manaan - West Central" 3 metres west; log "open door via WalkTo(coord)", "settled within reach, gap 0.00m, moved=0"). The player stood in the doorway and did not get into the lift. Workaround: select the exit itself and press Enter. Proposed fix, next to `RedirectLandmarkToTransition` in interact_dispatch.cpp: an open door with a transition within a few metres behind it passes Enter on to that transition.
- Enter on a closed door the player stands tight against sometimes does not arrive (17 Nov 2026, the Pressure Doors in the Sith base manm27aa: "Open Pressure Door, locked, may open when you try" and then nothing; log "walk never started", a sidestep). One step back and then Enter worked every time. Not investigated.
- A force field that counts as cosmetic but does close off the corridor (17 Nov 2026, Sith base manm27aa: "Force Field, cosmetic" at (177.2,11.3), static and locked, in the middle of the region between Door 12 and Door 13). In the mod's layout the region runs on at both sides of it: the region walk to Footlocker 5 took the player up to the field and then pressed Enter on Door 13 behind it, at which the game sent the character 45 metres the other way (log patch-20261117-*.log around 22:08Z, "explore walk: hop 6 region 81 -> 102 through a door"). The player also heard "Assault Droid Mark IV 5; 6" as present while they stood behind the field and did not attack. Proposed fix: a static door that is closed and locked and has floor on both sides is a barrier and not cosmetic; test this generally with the floor on either side, as for the place in front of a door.
- A gate that opens on use and falls shut again (Main Gate in the Undercity): the mod does not walk through it and the routes see it as locked. Workaround: the bridge's kotor.py gebruik HANDLE command, wait 2.5 seconds, then the bridge's kotor.py loop command to a point 8 metres behind it. Proposed fix: wait until it is open and then walk through.
- Shift+- on a closed door still sends UseObject, which at some doors ends two metres early and leaves the door closed (Davik's Estate, Sith Base). Enter does work. The region walk was fixed for this (102b8748); Shift+- itself was not, because the player does not use it on doors.
- A transition that begins while the party stands far behind hangs on a black screen (18 Nov 2026, Docking Bay manm26ad, transition to the Ebon Hawk; HK-47 and Canderous stood 77 metres back at the Assault Droid and did not follow). The mod said "Waiting for HK-47", "Walking on without HK-47" and walked into the trigger; log "ContentChange: ... kind=AreaTransition" and "FadeUnstick: obscuring fade held (alpha=1.00) with world input". After that the transition does not fire any more, not with everyone standing by it and not after walking out and in again; the player does not notice the black screen. Workaround that worked: F4, F5 (loading takes the black away), the bridge's kotor.py loop command a few metres out of the trigger, then Enter on the exit again. Proposed fix: do not walk into a transition trigger while someone stands further off than the game allows, but say who stays behind; and at a held fade say that a load is needed. Why the followers did not come along was not investigated (just before, the player tried "Dialog Assault Droid" with HK-47 as leader, which did not open; FootstepSup then kept reporting "stuck=1" for both followers).
- The lift in Upper City North: after "Elevator opens" the label stayed on "locked, may open when you try"; the next line should say "open".
- The cosmetic door ptar_static in the Hidden Bek Base is in Q/E although nobody can get to it. Its mark: it stands in no room at all (`GetRoomAtIndexed` returns -1). Safe form of the fix: let that count only when the same call does find the player's room at that moment, because a real door must never disappear.
- Enter on the map note of the basket to the Shadowlands only walks to the note and nothing happens (18 Nov 2026, Great Walkway kas_m22ab). After the uprising the basket works through a trigger without a transition flag (kas22_basketdown, y 100.6 to 106.7, script k_pkas_gorwookdn) three metres north of the map note "Basket to the Shadowlands" at (301.5,97.5). Workaround: the bridge's kotor.py loop 301.5 103.8 command, then the basket's dialog opens. Going up in the Upper Shadowlands (kas_m24aa) it is the same: the note "Basket Up" at (68.4,148.3) does nothing and the trigger kas24_basketup did not fire after walking into it either; what worked is Enter on "Basket" in the list of containers (tag kas24_baskettalk, at (68.4,147.1)): the bridge's kotor.py gebruik HANDLE command opens the dialog. Proposed fix: a note with a script trigger or a talkable object right behind it passes Enter on to that. The shipped rule for a note that carries the name of the thing beside it (dea8bfae) is to be tested here; see Monitor.

### Found in play 11 to 20 Nov 2026: walks that do not start or stall

- A Tomb Door that can be reached from one side only: 24 seconds of silence before the walk to it starts (Korriban, Tulak Hord; the player stood in the urn room). Enter gave "Open Tomb Door", then "walk never started", sidesteps, and only after 24 seconds the route walk; in that silence the player left the game window. Solved at the time with the bridge's kotor.py gebruik command on the door. Background: a sarcophagus behind a Tomb Door (Korriban: Ajunta Pall, Tulak Hord, Marka Ragnos) needs Enter on the Tomb Door first and only then on the sarcophagus; the mod now names the closed door at "Unable to reach target!" (d11f1d4c, not yet confirmed, see Monitor). What remains is the long silence; see the entry on the long walk further down this list.
- Followers do not come along round a turn-back zone (the edge of a desert; b0668c12). For the character the player controls the zones work: confirmed on 15 Nov 2026 in the Eastern Dune Sea ("Going round.", four route walks that arrived). The followers stayed on the point where the script puts the party back (205.7, 234.6), 200 metres behind the player, who fetched them one by one with Tab and Enter on the target (log patch-20261115-222311.log, 22:44Z to 22:48Z). Since eeddeb13 the mod says on arrival who stayed behind. Still to do: let the followers walk the same route. Also, on arrival at a container another Enter is still needed; only talking and use actions are repeated.
- Enter on someone behind a counter does not get going (15 Nov 2026, 18:37, Czerka Office tat_m17ag, Greeta Holda at (-5.81,0.26), player at (6.43,3.72)). The point "in front of the target" lies beside the floor, the floor repair of c21be77f picks the floor 1 metre further on, and the game does not start that walk (log "walk never started", three sidesteps "rooted", two Enters in a row with the same course). Suspected, not investigated: that nearest floor lies on the target's side of the counter. The log is patch-20261115-173647.log from 17:37:29Z.
- A walk to a point more than 5 metres from any floor goes to the game unchanged; the game drops such a walk and can crash on it.
- A container behind a closed door that the mod counts as part of the player's own region (14 Nov 2026, Jedi Enclave danm13: Footlocker at (54.8,69) behind Door 6 at (47.9,69); the mod said "Here: ... Footlocker" and "Door 6 East, to a dead end, new"). Enter on the container gave "walk never started", three sidesteps and then silence, because the region walk sees no door on the way when target and player lie in the same region. Workaround: Enter on the door first, then on the container. Proposed fix: determine the region of a target from the side of the door it lies on, or at "walk never started" check whether a closed door stands between player and target.
- A door the mod knows no route to and that is too far for one walk (18 Nov 2026, Hrakert Rift manm28ad: Pressure Door 1 at (73.6,122.3) from the Star Map at (190,107), 117 metres): "Open Pressure Door" and then "Movement cancelled, way blocked" (log "WalkTo: ... is off the walkable floor; walking to the floor nearest it, 1.48m away", "walk never started"); the other door did get "via West, about 115 metres to walk". Workaround: the bridge's kotor.py loop command in stretches of 25 to 35 metres: (160,107), (128,107), (102,108), (94,104), (84,99), (79,110), (74,120). To investigate: why this door gets no route while the floor beside it can be reached.
- A long walk the game does not start takes too long to get going (15 Nov 2026, Sandral Grounds, from the front door to the back door, 24 metres as the crow flies and 100 metres to walk): first "walk never started", then a coordinate walk, then three sidesteps "rooted", and only after that the route walk in hops, which did arrive (log "route walk: over at hop 14/14"). In those ten seconds of silence the player pressed other keys and thought the door did not open. Since fb555577 the mod says "Trying another way." after the first two seconds; the order of the attempts has not changed. Still to do: when the route is long, go to the route walk at once, without sidesteps.
- The game's own pause at "enemy sighted" (log "Pause: fire ... caller=0x004ba62d") does not go away when the window gets the focus back, and the bridge's kotor.py toets spatie command did not lift it; a Space key sent as real keyboard input (radix_input with key space) did.

### Found in play 11 to 20 Nov 2026: region layout and route text

- `DoorSides` still guesses two sides for an open door into another area; the route text and the overview use that.
- When the player arrives standing on the transition itself, every route text runs through that transition (16 Nov 2026, Lower Shadowlands kas_m25aa at (184.8,241.5), 1 metre from "To the Upper Shadowlands"): "Freyyr, 9 o'clock, 86 metres, via Transition East to To the Upper Shadowlands, about 155 metres to walk", and Ctrl+N names as the nearest unexplored place "Transition East ..., 1 metres". The player concluded that everything lay in the other area. Not investigated; Ctrl+Shift+N could walk the player back into the previous area there. Log patch-20261116-13*.log after 13:45Z. Again on 16 Nov 2026, 23:45, in the Docking Bay of Manaan (manm26ad, at (260.9,177)): everything "via Transition South to Ebon Hawk".
- A small hall the mod does not know as a region of its own (between the Standard Door and the lift, upstairs in tar_m09ab; log "FAILED walkmesh-shape gate ... filtered dead end"): standing there, the mod thinks the player is still in the region before it (Ctrl+N said "Junction"). The consequences for the lift door are fixed (864354d0); the layout itself is not.
- The exit of the hideout (tar02_aptdoor in tar_m02af) has no transition flags; Ctrl+N names it as "Way out", the region announcement does not yet.

### Found in play 11 to 20 Nov 2026: party and combat

- Controlling a companion who is not in the fight, the player hears nothing of the fight (Selven). The mod should at least say that the party is fighting and how it ends.
- Every hit of the party does 0 damage ("Hit ... for damage 0" without "Damage Breakdown", while the enemy does deal damage). It happens as long as the player character still has a dialog or a use ahead of it while there is fighting, and lasts until that dialog has been opened and closed again. Seven fights from 13 to 16 Nov 2026 in the logs (found with a script that lays, per log, the runs of 0-hits next to the Approach and Interact lines; droids and shields give real zeros too, which do not count). Fixed for what the mod itself sends in fc306ce4 (`TickApproach` in guidance_approach.cpp): when the party is fighting, the mod breaks off a walk to someone to talk to or something to use and says "Movement cancelled, combat." with the name; doors, exits and plain walks go on. Not yet confirmed in play; see Monitor. Not covered: a dialog that a script starts in the middle of a fight (that is the game's own and protects whoever surrenders), and a use that had already arrived before the fight began (14 Nov 2026, 12:31Z, a locked Footlocker). Why the game then gives zeros has not been worked out; the game's "dialog active" state (`SetGlobalDialogState` in engine_panels_state.cpp) is the suspected link. Workaround if it happens again: finish the dialog or close it with Escape.
- A dialog that a scripted object starts and that does not open locks everything afterwards (18 Nov 2026, Hrakert Station manm28aa: Enter on "Whimpering Locker", OnUsed k_pman_survive10, the same pattern of fade, moving the party and ActionStartConversation as the basket on Kashyyyk). No dialog opened, and after that the game accepted no use or open at all: "Use Environmental suit" and "Open Door" on Door 12 did nothing (log "UseObject dispatch ... ret=1", "no action enqueued (grace)", "walk never started"); only walking still worked. The player concluded that the door and the suit were broken. Workaround that worked: F4 and F5; after the load the hanging dialog opened by itself and everything worked again. This is presumably the same "dialog active" state as in the zero-damage entry above. Proposed fix: recognise that a dialog is hanging (`SetGlobalDialogState` in engine_panels_state.cpp) and say so, or open the dialog after all.
- The Select Party Members screen lets the mod's Q/E and comma/period through; it should hold back the world keys (`WorldInputOver` in engine_panels_state.cpp).

### Found in play 11 to 20 Nov 2026: screens, items and the bridge

- At the workbench the id in a row is a server id, while `ResolveItemFromRow` treats it as a client id (noticed 15 Nov 2026, not touched).
- The powers screen says "available" at every power, also when "Remaining Powers" is at 0; the player concluded there was still a choice to make. The number of remaining choices should be said with it. Investigated on 20 Nov 2026: the grid (menus_skillflow_grid.cpp) reads only the status per cell; the label of pwrlvlup that carries the number of remaining choices is not a known member of the panel (only SUB_TITLE_LBL, LBL_POWER, the lists and the buttons are mapped). Needed: put that screen's labels in the log in the game at a level-up and see which one carries the number.
- A container the game itself has no action for (18 Nov 2026, Hrakert Station manm28aa: "Remains" at (166.1,122.0), placeable invisible003, Useable 1 in the template, holding a Jedi Master Robe and a Crystal, Sigil; a dead "Dead Soldier", man28_deadjed, lies on top of it). Enter says "Use Remains" and does nothing (log "UseObject dispatch ... ret=1", "no action enqueued (grace)"), Shift+Enter says "No actions available for Remains" (log "Picker: empty descriptor ... descriptor_count=0", so the game's own radial menu is empty), and Ctrl+L and "To search" keep sending the player to it. Other containers opened normally at that moment. Cause: in the save this one object stood at Useable 0, while the template says Useable 1; what makes the game set that field to 0 was not found (no script, no Override file, the mod only reads the field). With the field at 1 the container opened normally (confirmed in play, 18 Nov 2026). In the whole save it was the only object with an inventory and Useable 0. In the mod: `IsUsablePlaceable` (engine_area_kinds.cpp) calls an object with an inventory usable also when Useable is 0. Proposed fix: have such a container say "cannot be opened" and keep it out of Ctrl+L and "To search".
- A container that was bashed open does not open after that, not through the game's own click either. Proposed fix: recognise such a container and say so.
- The bridge's /menu: the label of a completed level-up step reads as "?".

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

### Seen in play 11 to 20 Nov 2026, cause unknown

- Standard Door 6 and 7 in the garage did not open on Enter eleven times on 13 Nov 2026, although the game received the click; they were not locked. Later they stood open. Cause unknown; the screen showed "Action added to queue. Press the pause button to continue" at the time.
- Two exits with the same door number in one region announcement (seen at end_door15); check at the next case.
- The game's leader and the character the player controls can drift apart: the member table said Mission while the player controlled the player character, and the followers followed Mission ("Tab swapped nobody" in the log). How it arises is unknown. Recovery by hand: walk to the others and press Tab until the round is complete. If it happens again: read the engine's member table live while the game runs.
- A new party member can be missing from the list until the first load (Mission). Workaround: save and load after a new companion joins. Look in the log for "Party: member ... has no live creature".
- After the dialog of a story fight the fight twice did not get going (15 Nov 2026, Eastern Dune Sea; patch-20261115-202916.log around 21:14Z): the enemies stood still for two minutes without shooting, the player character stayed put while every Enter put an attack in the round, "Combat begins" and "Combat ends" alternated, and the action queue jumped between 3 and 5 every frame. On the third attempt it went right by itself. Cause unknown. If it happens again: do not load but look live (the bridge's kotor.py status command, /objects, and a kotor.py loop as a test of whether the character can move).
- Labels can shift after a load: sequence numbers depend on position, but moving creatures and new objects shift them. Compare in the log when there is a concrete example.

## Planned

### Nameable personal map pins

Let players give their own map pins a name when they drop one, and read that name back when cycling to the pin. Personal pins currently announce only a generic label plus position, so a player who marks several spots can't tell them apart. Needs a text-entry path that works from the keyboard with a screen reader (the editbox handling shipped for chargen + save naming — `menus_editbox.cpp` — is the closest existing mechanism, though map pins aren't an engine editbox so the entry surface differs), storage of the label alongside the pin in the save, and the cycle/focus announcements updated to speak it. See `map_user_markers.cpp`.

### Bundle the resolution / widescreen patch — once we are out of beta

Offer the community resolution / widescreen + HR-menus patches as an installer option, and make our own code robust to the layout changes they introduce. Deliberately deferred until the mod leaves beta: it widens the support surface (several of our paths still assume a particular screen geometry — screen-pixel coordinates, the OS cursor, hardcoded hit-test offset compensations), and a beta is the wrong time to add a second variable to every bug report. Prerequisite when we do take it on: audit every place we depend on screen geometry and read it live from the engine instead of assuming. See `archiev/installer-design-original.md` (Widescreen / HR-menus bundling).

### Not yet built: ideas from play, 11 to 20 Nov 2026

- Waiting for the party at a transition walks to the goal in one go once the wait is over; where the game refuses long walks, that should go through the route walk. The wait also does not yet apply to Shift+-, Ctrl+Shift+N and walking by hand.
- Say the reason when there really is no route. Not built yet (YAGNI until it bites).
- A movie (a .bik; in the log "Loading: STUNT_" and "movie window on screen") says only "Movie." since 409fce92; nothing says what there is to see.
- Enter on an item that cannot be used (a datapad) should read the description instead of only the game's own message. Investigated on 20 Nov 2026: no log holds what the game says at Enter on a datapad, so there is no text to hook on to; Shift+Down arrow does read the description. Needed: Enter on a datapad in the inventory once, then the message is in the log.
- Enter on a door that has to be opened with Security should choose Security by itself (hard: radial menu and action bar).
- Finding someone by name whom the player has not heard yet: the bridge's kotor.py zoek NAME command finds everyone; in the mod itself this works only through Map-wide object selection. Build further if wanted.
- The bridge's status and rond commands for KOTOR 2.

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

### To confirm in play: fixes of 15 and 16 Nov 2026

- The place right in front of a door belongs to the side you stand on (1b43996f, 16 Nov 2026). To test in the village on Kashyyyk in front of the door "Chieftain's Hall" at (94,67): Ctrl+N should name the exits of the village there and not "behind locked doors". Log: "ExitMemory: region ... is the doorway sliver of door". Watch for the reverse too: if the mod says somewhere that something can be reached while a locked door stands in between, the margin of 0.3 metres in `FindDoorOnEdge` (room_topology_doors.cpp) is too wide. The check script laid the rule next to all doors from a week of logs: 158 of the 168 do not change.
- "Movement cancelled, combat." with the name, as soon as there is fighting while the player is on the way to someone or something (fc306ce4, 16 Nov 2026). Log: "Approach: the party is fighting — released". Watch whether it breaks off too often (a Tach in the distance) and whether the player can then still walk and attack normally.
- Enter on a far target the game makes no action for (the Force Field in kas_m24aa from 120 metres and further) should now say "Walking to ...", walk there and give Enter there by itself (bcbb3d69, 16 Nov 2026). Log: "Picker: descriptor belongs to ... has no copy" (or "has a copy": then the suspicion that the client does not know the object at that distance is wrong) and "Interact: ... the region walk goes there and acts on arrival". If it says "Dialog" or "Open" with the name of the far target again and the character walks the wrong way, it is not solved; the workaround is Shift+- on the target and Enter once there.
- False Pazaak (fix of 15 Nov 2026): no "Pazaak. Press C..." any more in a dialog, and at a real game still "Pazaak: acquired board panel=".
- A loose item in the world (fix of 15 Nov 2026): Enter says "... cannot be picked up", and Ctrl+L and "To search" no longer name it.
- A key in the inventory (fix of 15 Nov 2026): "locked, you have the key". Log: "LockNeed: ... -> 5". The two engine calls behind it (GetItemRepository, FindItemWithTag) have never yet run in the game.

### To confirm in play: first round of 20 Nov 2026

- Health (9589e49d): in a fight "korasha sinks to 150 of 323" at half and at a quarter, and the quarter interrupts; Ctrl+H and the summary say "Canderous down". Watch whether it talks too often and whether the maximum of companions is right. Log: "PartyStatus: health warning, band N".
- Wordless scene (7eab047f): "Scene without words." after 2.5 seconds without a line and without replies, "Scene over." when the panel closes. Watch whether it fires in the middle of an ordinary dialog. Log: "Dialog.Speech: wordless scene named".
- Closed door by the target (d11f1d4c): "A closed Tomb Door stands by it." after "way blocked" and after "Unable to reach target!".
- Equipment list (ca9ea23a): the lines with "--" are no longer said, and only what changes is said.
- Bridge (577e4a12): /state gives dialog_replies as long as a dialog is open, at a computer too.

### To confirm in play: second round of 20 Nov 2026

- "Combat ends" with the party line when someone is down, also without kills (bf4ebc75).
- The load menu says ", newest" after the save with the longest play time (9cd5db6b). It works only if the row shows a play time such as "80h 24m", otherwise it says nothing: to check.
- The workbench's list of items says ", upgraded" (36cf27b3). Log: "WorkbenchItems: ... upgraded=1".
- A use that ends at a closed door between the player and the target says "A closed Door stands by it." (537e9838, the urn of Tulak Hord).
- A walk that does not start says "Trying another way." after two seconds (fb555577).
- Enter on a map note that carries the name of the person or thing within 3 metres goes to that target (dea8bfae). Log: "landmark [...] stands ... from its namesake". To test at Freyyr, the Computer and the basket "Basket Up" on Kashyyyk.
- A walk that arrives while a companion is more than 40 metres away says "HK-47 stayed behind." (eeddeb13). Watch whether it talks too often in solo mode.
- A movie in the game says "Movie." (409fce92); not at the movies of the start-up.

### To confirm in play: list stepping at the workbench and in the equipment list (a8faa3a1 and 69c32401, 20 Nov 2026)

- In the workbench's crystal list and in the equipment list, Up and Down should step from the row the keyboard was on, and Enter and Shift+arrow should take that row. The game puts the selection back every frame on the row under the mouse cursor, and moving the cursor away does not always hold; because of that the list stuck on "Crystal, Firkrann, 4 of 7" on 20 Nov 2026 and the Solari crystal could not be chosen. Log on a deviation: "steps from the keyboard's row N, not the engine's M (hover-select)". If it still fails: scroll the list and click the row with the mouse (radix_input, with the coordinates from the screenshot times 1.5), then Down to Assemble and Enter. Mouse clicks do work in the game's menu screens (workbench, Select Party Members), not in the world.

### Swoop race automatic pilot (KOTOR 1 only)

- The setting "Swoop race: automatic pilot" steers to the line, shifts by itself and avoids buoys; the player presses nothing. KOTOR 1 only. It has been run on the Taris, Tatooine and Manaan (18 Nov 2026) tracks. A race without the line "race timer start" in the log had no signals and no pilot.
- After a race on a new track, look in the log at "SwoopRace" and "SwoopAssist": "pilot: shift", "CROSS ... hit=", "pilot: buoy slot=", and "box=FAILED" (then the pilot works with the origin of the buoy instead of its collision box). A collision shows as a drop in "snapshot speed".

### Still watching from play, 11 to 20 Nov 2026

- The kill announcement no longer interrupts speech. If the player finds that speech lags behind in a long fight, let the kill interrupt again and keep only the summary.
- Check the container fields: LockNeed lines with kind=9 in the log against `python kotor-tools\check_locks.py MODULE`.

## Polish

### Starting an autowalk while one is active should switch, not cancel

If an autowalk is already running and the player starts a new one on a different target, the current behaviour just cancels the existing one. Instead it should immediately start the new walk — switching the route in one gesture rather than requiring cancel-then-start.

### Mod-settings sliders only save after pressing Enter

The sliders under Mod settings (e.g. hint-sound volume) only persist their new value to `acc_settings.ini` once the user presses Enter on the row. Adjusting a slider with Left / Right changes it for the session and previews the new level, but the change isn't written until an explicit Enter — so a player who tweaks a slider and leaves the menu without pressing Enter loses the change on next launch. Slider adjustments should persist on each Left / Right step (the same way the value already updates live), without needing a confirming keypress.

### Small wording and comfort points from play, 11 to 20 Nov 2026

- When a command gets stuck through circling (log "StuckProbe: circling detected") the mod only says "Clear: ..."; that should become a clear message.
- Ctrl+Shift+N in places with many small pieces walks only 5 to 10 metres per press; possibly let it walk on until a door, loot or a person.
- "You must gather your party before venturing forth." is read as a list of one row ("..., 1 of 1"); Enter closes the window. Until it is closed it holds back Tab.
- Let a computer panel start at the top, or say the number of choices first.
