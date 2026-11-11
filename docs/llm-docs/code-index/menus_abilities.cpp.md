# menus_abilities.cpp (459 lines)

Source: `patches/Accessibility/menus_abilities.cpp`

In-game Abilities screen — see menus_abilities.h for the design rationale.

## Declarations (in source order)

- L32 `void* SelectedRow(void* panel)`: The ability_listbox row at the current selection_index.
- L44 `void CallOnEnterSkill(void* panel, void* row)`: CSWGuiInGameAbilities::OnEnterSkill(this, CSWGuiControl* row) — the engine's per-skill repaint.
- L59 `void CallPanelInput(void* panel, int code)`: Feed a panel-internal code to CSWGuiInGameAbilities::HandleInputEvent.
- L71 `void* ChartForTab(void* panel, int tab)`: The panel's embedded CSWGuiSkillFlowChart for a chart tab.
- L80 `bool ChartCanStep(void* panel, int tab, bool down)`: True iff the Feats/Powers chart can step one row in the given direction WITHOUT wrapping.
- L98 `unsigned int ChartCellId(void* chart, int row, int col)`: Feat/power id held in chart cell (row, col), or kFlowSkillStructEmptyFeatId for an empty cell / any read failure.
- L138 `void SelectMaxRankInRow(void* panel, int tab)`: Point the chart at the highest-ranked cell of its current row, then repaint the name label + description for it.
- L191 `bool PowersAvailable(void* panel)`: True iff the Powers tab exists for this character (Jedi with powers).
- L210 `void SwitchToTab(void* panel, int tab)`: Switch directly to a specific tab (0 Skills / 1 Powers / 2 Feats).
- L224 `int BuildAvailableTabs(void* panel, int outTabs[3])`: Ordered list of available tabs: Skills, [Powers if any], Feats.
- L234 `int ReadTab()`: Active tab: 0 = Skills, 1 = Powers, 2 = Feats (CGuiInGame.field139_0xbc0).
- L245 `bool ReadLabel(void* panel, size_t offset, char* buf, size_t bufSize)`: Read an inline CSWGuiLabel/CSWGuiButton member at panel+offset.
- L256 `void AppendPair(void* panel, size_t labelOff, size_t valueOff, char* msg, size_t msgSize)`: Append ", " to msg iff the value label is non-empty.
- L284 `bool TabHasStats(int tab)`: Speak the shown entry: LBL_NAME, plus the rank/bonus/total pairs on every tab EXCEPT Feats.
- L286 `void AnnounceDetail(void* panel, const char* prefix, bool withStats)`
- L309 `void SpeakTabName(void* panel, int tab)`: Speak just the tab name (its button text) — used at the tab level, where the user is choosing which tab to drill into.
- L328 `bool IsAbilitiesPanel(void* panel)`
- L334 `void RefreshDetail(void* panel)`
- L344 `void BrowseList(void* panel, int tab, ListBoxNavOp op)`: Browse one entry on the current tab's list (drilled state).
- L371 `bool HandleInput(int /*n*/, void* /*thisPtr*/, void* activePanel, int param_1, int param_2, int& outRv)`
