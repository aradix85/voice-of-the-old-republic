# menus_powers_levelup.h (59 lines)

Source: `patches/Accessibility/menus_powers_levelup.h`

Force-power picker (CSWGuiPowersLevelUp).

Used by both the chargen Power-selection screen and the InGameLevelUp "Kr�fte" sub-screen — the same engine class hosts both flows (swkotor.exe.h:16603, decompile of OnPowerSelectionChanged @0x006f1940).

The panel is structurally a 2D skill tree, NOT a flat listbox: its powers_listbox (.gui id 6 on K1, 12 on K2 — this panel's control ids are re-numbered between the games, see the id table in the .cpp) holds CSWGuiSkillFlow rows (one per power family), each with up to 3 CSWGuiFlowSkillStruct cells (base / improved / master variants) — identical layout to chargen Talente. The chart at kPowersLevelUpChartOffset (0x19fc K1 / 0x1bf8 K2) is a CSWGuiSkillFlowChart tracking (row, col) selection state.

Our navigation model mirrors menus_chargen_feats: a 2D cursor over non-empty cells, vertical movement keeps the column (snapping to the nearest filled column when the destination row doesn't have it), horizontal movement steps through filled columns within a row, then virtual rows for the action buttons. Enter on a cell dispatches OnPowerPicked(powerId); Enter on a button activates that button; Esc queues BTN_BACK.

Engine state changes per focused cell: * CSWGuiSkillFlowChart::SetSelectedSkill(chart, powerId) — chart render highlight + (selected_row, selected_col). * CSWGuiPowersLevelUp::OnEnterPower(panel, powerId) — refreshes power_label + description_listbox + BTN_SELECT state for the focused power.

## Declarations (in source order)

- L37 `bool IsPowersLevelUpPanel(void* panel)`: True iff `panel` is the CSWGuiPowersLevelUp screen (identified via engine_panels::PanelKind::PowersLevelUp).
- L44 `const char* GetTitleOverride(void* panel)`: Title-override hook for AnnouncePanelTitle.
- L55 `bool HandleInput(int n, void* thisPtr, void* panel, int param_1, int param_2, int& outRv)`: Input dispatcher.
