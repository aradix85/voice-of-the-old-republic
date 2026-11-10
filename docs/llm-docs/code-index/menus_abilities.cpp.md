# menus_abilities.cpp (475 lines)

Source: `patches/Accessibility/menus_abilities.cpp`

In-game Abilities screen — see menus_abilities.h for the design rationale.

## Declarations (in source order)

- L31 `void* SelectedRow(void* panel)`: The ability_listbox row at the current selection_index.
- L53 `void CallOnEnterSkill(void* panel, void* row)`: CSWGuiInGameAbilities::OnEnterSkill(this, CSWGuiControl* row) — the engine's per-skill repaint.
- L68 `void CallPanelInput(void* panel, int code)`: Feed a panel-internal code to CSWGuiInGameAbilities::HandleInputEvent.
- L80 `void* ChartForTab(void* panel, int tab)`: The panel's embedded CSWGuiSkillFlowChart for a chart tab.
- L89 `bool ChartCanStep(void* panel, int tab, bool down)`: True iff the Feats/Powers chart can step one row in the given direction WITHOUT wrapping.
- L107 `unsigned int ChartCellId(void* chart, int row, int col)`: Feat/power id held in chart cell (row, col), or kFlowSkillStructEmptyFeatId for an empty cell / any read failure.
- L147 `void SelectMaxRankInRow(void* panel, int tab)`: Point the chart at the highest-ranked cell of its current row, then repaint the name label + description for it.
- L203 `bool PowersAvailable(void* panel)`: True iff the Powers tab exists for this character (Jedi with powers).
- L222 `void SwitchToTab(void* panel, int tab)`: Switch directly to a specific tab (0 Skills / 1 Powers / 2 Feats).
- L236 `int BuildAvailableTabs(void* panel, int outTabs[3])`: Ordered list of available tabs: Skills, [Powers if any], Feats.
- L246 `int ReadTab()`: Active tab: 0 = Skills, 1 = Powers, 2 = Feats (CGuiInGame.field139_0xbc0).
- L261 `bool ReadLabel(void* panel, size_t offset, char* buf, size_t bufSize)`: Read an inline CSWGuiLabel/CSWGuiButton member at panel+offset.
- L272 `void AppendPair(void* panel, size_t labelOff, size_t valueOff, char* msg, size_t msgSize)`: Append ", " to msg iff the value label is non-empty.
- L300 `bool TabHasStats(int tab)`: Speak the shown entry: LBL_NAME, plus the rank/bonus/total pairs on every tab EXCEPT Feats.
- L302 `void AnnounceDetail(void* panel, const char* prefix, bool withStats)`
- L325 `void SpeakTabName(void* panel, int tab)`: Speak just the tab name (its button text) — used at the tab level, where the user is choosing which tab to drill into.
- L344 `bool IsAbilitiesPanel(void* panel)`
- L350 `void RefreshDetail(void* panel)`
- L360 `void BrowseList(void* panel, int tab, ListBoxNavOp op)`: Browse one entry on the current tab's list (drilled state).
- L387 `bool HandleInput(int /*n*/, void* /*thisPtr*/, void* activePanel, int param_1, int param_2, int& outRv)`
