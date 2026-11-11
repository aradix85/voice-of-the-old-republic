# menus_chain.cpp (363 lines)

Source: `patches/Accessibility/menus_chain.cpp`

The menu chain: one flat, top-to-bottom list of a panel's navigable controls that the arrow keys walk. Here: the checks that the bound panel is still the same live panel, and RebindChain, which builds the list.

## Declarations (in source order)

- L27 `bool IsModalTextPanel(PanelKind k)`
- L44 `void LogChainContents(void* panel, void* active)`: Per-rebind chain dump.
- L82 `bool IsPanelLive(void* panel)`: Panels[]-membership check, shared by Validate{Tabbed,Chain}Panel and the DetectTabsCluster precondition.
- L98 `void* ReadPanelActiveControl(void* panel)`
- L104 `int FindChainEntry(void* control)`
- L118 `void SpeakLevelUpDoStepFirst()`: Speak guidance when the user activates an InGameLevelUp category that the engine has not currently enabled.
- L146 `void ResetTabbedState()`
- L152 `void RebindChainPreserveIndex(void* panel)`
- L164 `void InvalidateChain()`
- L178 `void ValidateTabbedPanel()`
- L203 `bool ChainPanelIdentityHolds()`: Identity check for the panel the chain is bound to.
- L215 `void ValidateChainPanel()`
- L234 `void RebindChain(void* panel)`
