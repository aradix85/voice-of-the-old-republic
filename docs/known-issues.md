# Known limitations

What does not work, or works differently from what a player would expect, and why. Facts only: no test plans, no to-do lists, no reports from play. An entry leaves this file when the limitation is gone; the changelog and the commit are the record of the fix.

## Doors and locks

- **"Locked, may open when you try" is too wide.** Every door with an OnFailToOpen script gets it, also when the script only barks a line (Arena Door, cp_tar02_duelbrk).

## Finding your way

- **Exit words are measured from the junction's map point, not from the player.** A region label is built once per cluster, from its node or centroid (room_topology `ClassifyCluster`), and spoken when the player is near that centre or has stopped getting closer (transitions.cpp `CentreRuleReady`). A crossing that is a single node, passed at a distance — a wide street, an open outdoor area — still speaks its words from the node: a door named south-east can be east-north-east of the player. Labels were kept constant on purpose; words measured from the player would change as they walk.
- **A new autowalk cancels the running one.** Starting a walk to another target while one is active only stops the first; the second needs another press.

## Keys that act while a screen owns the keyboard

Three hotkeys are gated on "is there a world" alone (`GetPlayerPosition`), with no check for a foreground panel:

- **Shift+H, the combat queue** arms over any panel, and its rows call `RemoveLastAction` on the live combat round.
- **N, camera to an exit** synthesises the bound turn key through `SendInput`, so with a screen up those scancodes land in the screen.
- **Shift+L, level up** opens the engine's level-up wizard over whatever is in front; its own already-open and can-level-up guards keep it to one stacked panel.

The log names every in-world dispatch that runs over a foreground panel of an unlisted kind: `Interact: <key> GATE GAP?`. The AreaTransition banner is unlisted on purpose, so a press at an area transition logs one such line.

## Actions the game refuses

- **KOTOR 1 gives a reason, KOTOR 2 mostly cannot.** KOTOR 1 refuses an impossible action at the GUI layer and the entry's flag word carries a reason code, so the mod speaks the engine's own sentence (`ActionBar: variant flags ... usable=0 reason=N` in the log). KOTOR 2's item appender never initialises the flag word for inventory items — only the three equipped-slot entries get one — so what is there is heap garbage, and `VariantRefusal` skips the verdict for KOTOR 2 item entries. Force powers and feats do carry real flags in KOTOR 2.
- **A press that does nothing says "Not possible."** For the personal columns (4 to 7) the mod watches whether the press queued anything (`ArmNoOpWatch`, `TickNoOpWatch`); target rows act without a combat-round add and are not watched. In KOTOR 2 a use-item request that was sent (`Diag.UseItemReq`) counts as having acted.
- **KOTOR 2 medical items go around the engine's own flow.** The engine arms a target pick that only a mouse click on a party portrait completes, so from the keyboard the row could never fire. `SendUseItemRequestK2` sends the same use-item message the portrait click sends, and the action menu adds a party-member picker. A medical item the server then drops (full health) gets the generic "Not possible.", not a reason: the direct send skips the engine's client-side CanUseItem gate.

Code: `engine_actionbar.cpp`, `combat_queue.cpp`, `combat_diag.cpp`, `unified_action_menu.cpp`.

## Items from other mods

- **An item whose base item row does not exist loads as row 0.** The engine dereferences the baseitems row without a null check and would crash on the area load (a third-party `.uti` whose baseitems.2da row is missing from the effective Override). `baseitem_guard.cpp` clamps an out-of-range id to row 0 in both games and logs it under `BaseItemGuard`.

## Graphics

- **Grass is off.** The installer sets `Grass=0` in `[Graphics Options]` in both games, which avoids an access violation inside the Intel OpenGL driver, called from `GLRender::DrawLightmappedGrass`. The root cause is not fully pinned; the analysis is in `docs/grass-crash-analysis.md`.

## Menus

- **KOTOR 1: a run of panel labels is spoken on the load into a new game** ("Add Power", "Buy", "Party Inventory", "Options" and others), in the second the panels are built, before "Area: ...". KOTOR 2 does not have it.
- **Mod-settings sliders are saved only on Enter.** Left and Right change a slider for the session and preview the level; `acc_settings.ini` is written when Enter is pressed on the row.

## Minigames

- **The swoop race automatic pilot is KOTOR 1 only.** It steers to the line, shifts and avoids buoys. A race without the line "race timer start" in the log had no signals and no pilot; "box=FAILED" under `SwoopAssist` means the pilot used a buoy's origin instead of its collision box.
