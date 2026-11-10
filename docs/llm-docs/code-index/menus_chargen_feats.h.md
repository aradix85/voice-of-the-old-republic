# menus_chargen_feats.h (60 lines)

Source: `patches/Accessibility/menus_chargen_feats.h`

chargen "Feats" main panel (CSWGuiFeatsCharGen).

The screen is a 2D feat-tree chart: a CSWGuiSkillFlowChart of N rows × up to 3 columns, embedded as the single child of feats_listbox. Each row is a feat-progression chain (col0 root → col1 successor → col2 master); each cell carries (feat ID, status byte). The user picks feats by clicking cells with the right status — Add for available, Remove for chosen-this-level — and the same Add/Remove button reflects the cell's add/remove affordance.

This panel doesn't fit the listbox-driven dispatcher in menus_listbox.cpp (the chart isn't row-shaped — DriveListBoxSelection would manipulate the listbox's selection_index but the listbox has a single child). It gets its own input handler that builds a flat row-major cursor over non-empty chart cells + 3 trailing button virtual-entries, navigated with K/L. The three are ordered as the panel lays them out left to right, which the two games mirror: KOTOR 1 is Recommended / OK / Cancel, KOTOR 2 Cancel / OK / Recommended. Each entry carries the engine-member resolver for its button (FeatsPanel*, menus_internal.h) — the .gui ids they used to carry survive only as GuiIdMismatch tripwires inside those resolvers.

Engine state changes we drive from the keyboard: * On per-cell focus: SetSelectedSkill(featId) + OnEnterFeat(featId) — refreshes name_label, description_listbox, and the chart's visual highlight in one round-trip. * On Enter against a chart cell: OnFeatPicked(featId) — engine dispatches DetermineFeat + AddChosenFeat / RemoveChosenFeat / "you can't change this" message box, then BuildButtons rewrites statuses on every cell. * On Enter against a button virtual-entry: QueueControlActivate on the ctor-bound member the entry's resolver returns. * On Esc: QueueControlActivate(FeatsPanelBackButton).

Cell statuses (chart enum, lowest byte at FlowSkillStruct +0x120): 0 available, 1 existing, 2 granted-this-level, 3 locked (default — prerequisite chain not yet satisfied), 4 chosen-this-level.

## Declarations (in source order)

- L44 `bool IsChargenFeatsPanel(void* panel)`: True iff `panel`'s vtable matches CSWGuiFeatsCharGen.
- L56 `bool HandleInput(int n, void* thisPtr, void* panel, int param_1, int param_2, int& outRv)`: Input dispatcher.
