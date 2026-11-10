# menus_crafting.h (136 lines)

Source: `patches/Accessibility/menus_crafting.h`

KOTOR 2 workbench / crafting item-list screens.

KOTOR 2 restructures the workbench: the category-select panel (CSWGuiUpgradeSelection, upgradesel_p.gui) carries the upgradeable-item list INLINE (LB_UPGRADELIST, five category filter buttons including BTN_ALL), and two new K2-only screens hang off it:

* CSWGuiCreateItem (component_p.gui) — workbench item creation + breakdown, opened by BTN_CREATEITEMS. Store-shaped: LB_SHOPITEMS (craftable templates) and LB_INVITEMS (inventory to break down) share one screen slot; the engine flips the visibility bit (CSWGuiControl.bit_flags & 0x2) on whichever is active, exactly like the store's buy/sell model. BTN_Examine toggles the views. * CSWGuiCreateMedicalItem (chemical_p.gui) — the lab-station twin (chemicals + Treat Injury instead of components + Repair).

Decompile evidence (K2): * BTN_UPGRADEITEMS handler 0x008c7f60 acts on the LIST'S SELECTED ROW (GetSelectedRow when invoked via the button; row's obj id at +0x1d0). * BTN_Accept handler 0x008d2150 (component) dispatches create (0x008d4ac0) or breakdown (0x008d4680) on the selected row, picked by the create-vs-breakdown flag at panel+0x3ee4. * Esc needs nothing from us: the ctors bind BTN_BACK / BTN_Cancel to the engine's native input code 0x62.

What this module adds for screen readers: chain Enter on a list row commits it ("select row, then fire the panel's own commit button" as a single deferred op — atomic within one drain tick, so the engine's per-frame hover-select can't retarget the selection in between), the chain skips rows of whichever item list is hidden, mode flips announce, list repopulation rebinds the chain, and first-sight titles come from the panel's own localized title label.

All entry points self-gate: they no-op on KOTOR 1 (the panel kinds only identify on K2 vtables; the WorkbenchSelect path checks IsKotor2).

## Declarations (in source order)

- L46 `bool ResolveRowCommit(void* panel, void* control, void** outListBox)`: If `control` is a row of the actionable item list on a K2 workbench screen (WorkbenchSelect's LB_UPGRADELIST, or the VISIBLE item list of a crafting screen), fill the owning listbox and return true.
- L53 `void DispatchRowCommit(void* panel, void* listBox, void* row)`: Drain-side worker for the CraftRowCommit pending op: re-verify the row still sits in the listbox, write the listbox selection to its index, then FireActivate (vtable[15], event 0x27) the panel's own commit button (BTN...
- L65 `void AnnounceChainStepSuffix(void* panel, void* control)`: Per-row price quote, spoken after the row's name on each chain step — the crafting counterpart of store::AnnounceChainStepSuffix.
- L72 `void* ViewToggleButton(void* panel)`: The K2 crafting screen's BTN_Examine ("View Inventory"), or nullptr when `panel` isn't one of the two crafting screens.
- L80 `void* CommitButton(void* panel)`: The screen's BTN_Accept ("Create Item"), or nullptr off the crafting screens.
- L86 `bool ToggleViewFromHotkey()`: Q / E on a K2 crafting screen: flip create <-> break down by firing the panel's own view button.
- L100 `void SyncSelectedRowFromChainFocus()`: Mirror chain focus into the engine's own listbox selection on a K2 crafting screen, so the panel's "Create Item" button acts on the row the user is standing on.
- L107 `bool IsHiddenCraftingListBox(void* panel, void* listBox)`: True iff `panel` is a K2 crafting screen and `listBox` is its currently HIDDEN item list (LB_SHOPITEMS or LB_INVITEMS with the visibility bit clear).
- L119 `bool OwnsPanelTitle(void* panel)`: True iff this module owns the panel's title speech: K2 only, for the whole workbench family (Select / Items / Upgrade / CreateItem / CreateMedical).
- L133 `void Tick()`: Per-tick watcher over the foreground K2 workbench screens: * announces the screen's title when a workbench-family panel ARRIVES in the foreground (replaces first-sight for these panels on K2 — see OwnsPanelTitle).
