# menus_chain.h (254 lines)

Source: `patches/Accessibility/menus_chain.h`

Chain-navigation state + helpers.

The chain is the menu mod's central abstraction: a flat, visually- sorted list of focusable controls on the current panel that arrow keys walk top-to-bottom.

What lives here: the chain array + cursor, tab-cluster detection, click-offset compensations (tab-y / equip-slot-y / class-icon-x), the geometry helpers that build + walk it.

## Declarations (in source order)

- L16 `struct ChainEntry`: One chain entry = one navigable target on the current panel.
- L113 `int ComputeTabClickOffsetY(void* panel)`: Tab-cluster Y pitch for cursor-warp hit-test compensation.
- L120 `void RebindChain(void* panel)`: (Re)bind the chain to the currently focused panel.
- L132 `void RebindChainPreserveIndex(void* panel)`: Same as RebindChain, but after the rebuild restore the chain cursor to the position it held before the call (clamped to chainCount-1).
- L145 `void InvalidateChain()`: Drop all chain state without rebuilding.
- L149 `void ResetTabbedState()`: Reset tabbed-mode state (tab cluster fields).
- L153 `void ValidateTabbedPanel()`: Per-tick guard against a dangling g_tabbedPanel.
- L164 `void ValidateChainPanel()`: Post-FireActivate guard against a dangling g_chainPanel.
- L178 `bool ChainPanelIdentityHolds()`: True while the object at g_chainPanel is still the panel we bound to.
- L184 `bool DetectTabsCluster(void* panel, int& outStart, int& outCount)`: Detect Options-style layout: CSWGuiListBox at controls[0] with controls.size > 0, followed by a contiguous cluster of buttons.
- L188 `bool IsTabButton(void* control)`: True if `control` is one of the current tabbed panel's tab buttons.
- L193 `void* FindAdjacentArrow(void* panel, void* focused, bool toRight)`: Find the empty-text navigable neighbour at the same y-row in panel.controls — the cycle-arrow flanker around a value-display button.
- L202 `void* FindCloseButton(void* panel)`: Heuristic finders for the back-out / cancel buttons used by the Esc handler.
- L203 `void* FindCancelButton(void* panel)`
- L206 `int FindChainEntry(void* control)`: Linear scan of g_chain for `control`.
- L210 `void* ReadPanelActiveControl(void* panel)`: Read panel.activeControl.
- L221 `void WalkChildren(const char* label, void* parent, size_t offset, const char* kindName = nullptr)`: Diagnostic dump of a parent's CExoArrayList children.
- L228 `void HandleEsc(void* activePanel, int code, int val, bool& consumed)`: Esc dispatch — routes press-edge Esc on chain-bound panels through the store override, the workbench-upgrade override, or the generic sub-dialog / modal-popup close path.
- L234 `void HandleLeftRight(void* activePanel, int code, int val, bool& consumed)`: Left/Right dispatch — slider in/decrement on focused sliders, cycle-arrow flanker activation otherwise (with a portrait-panel override).
- L241 `void HandleNavStep(void* activePanel, int code, int val, bool& consumed)`: Up/Down/Home/End chain step — advances g_chainIndex, runs the per-row announce + chargen sync + cursor warp pipeline, logs the step.
- L251 `void HandleEnterActivation(void* activePanel, int code, int val, bool& consumed)`: Enter on the focused chain entry — classifies the target (virtual / tab button / equip slot / workbench-upgrade slot / store item / journal row / text-only / default) and queues the right pending op (mod-settings open...

Also 11 constants or variables at namespace scope.
