# menus_chain.cpp (390 lines)

Source: `patches/Accessibility/menus_chain.cpp`

The menu chain: one flat, top-to-bottom list of a panel's navigable controls that the arrow keys walk. Here: its state, the checks that the bound panel is still the same live panel, and RebindChain, which builds the list.

## Declarations (in source order)

- L28 `bool IsModalTextPanel(PanelKind k)`
- L45 `void LogChainContents(void* panel, void* active)`: Per-rebind chain dump.
- L93 `bool IsPanelLive(void* panel)`: Panels[]-membership check, shared by Validate{Tabbed,Chain}Panel and the DetectTabsCluster precondition.
- L109 `void* ReadPanelActiveControl(void* panel)`
- L115 `int FindChainEntry(void* control)`
- L130 `void SpeakLevelUpDoStepFirst()`: Speak guidance when the user activates an InGameLevelUp category that the engine has not currently enabled.
- L158 `void ResetTabbedState()`
- L164 `void RebindChainPreserveIndex(void* panel)`
- L176 `void InvalidateChain()`
- L191 `void ValidateTabbedPanel()`
- L201 `uintptr_t ReadVtable(void* obj)`: Read a panel/control's vtable word under SEH.
- L229 `bool ChainPanelIdentityHolds()`: Identity check for the panel the chain is bound to.
- L241 `void ValidateChainPanel()`
- L260 `void RebindChain(void* panel)`
