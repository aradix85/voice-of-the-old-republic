# menus_chargen_layout.h (113 lines)

Source: `patches/Accessibility/menus_chargen_layout.h`

Layout + behaviour shared by menus_chargen_attr + menus_chargen_skills.

Both panels are the same screen with different data: an N-button + N-label + N-plus + N-minus grid (N = 6 abilities, 8 skills), a "points remaining" label, and a description listbox the engine repopulates from whichever points button it thinks is hovered. Only the offsets, the count and the log tag differ.

Everything that follows from that sameness lives here, parameterised by a PanelDesc the panel supplies once. The per-panel namespaces keep their public names (IsChargenAttributesPanel, AbilityIndexFromButton, SyncSelectedAbilityFromChainFocus, …) as thin wrappers, because they have callers across menus.cpp, menus_chain*, menus_focus, menus_monitors and menus_pending.

What deliberately does NOT live here is the genuine domain variation: the attributes panel computes a D&D modifier and a point-buy cost and tracks breakpoint crossings across four message variants; the skills panel has no modifier and a flat class/cross-class cost. Those stay in their own files — see the "not recommended for merging" note in archiev/refactoring/reports/phase-3-menus-chain-chargen.md.

## Declarations (in source order)

- L33 `struct PanelDesc`: Everything that distinguishes the two panels.
- L66 `bool IsBindingReady(const PanelDesc& d, void* panel)`: True when the panel's engine binding resolves — i.e.
- L70 `void LogBindingWord(const PanelDesc& d, void* panel, const char* when)`: Diagnostic dump of the panel's binding word (log-only, nothing branches on it).
- L73 `bool IsPanel(const PanelDesc& d, void* panel)`: True iff `panel`'s vtable is this panel kind.
- L77 `int IndexFromButton(const PanelDesc& d, void* panel, void* control)`: Index 0..count-1 if `control` is one of the panel's value buttons; -1 otherwise (not in range, not aligned to the stride, out of bounds).
- L83 `int RowPitchForCursorWarp(const PanelDesc& d, void* panel, void* control)`: Row pitch (~30-45 px) from buttons[0] / buttons[1] extents, or 0 if `control` is not a value button or the extents read implausibly.
- L88 `void SyncSelectedFromChainFocus(const PanelDesc& d)`: Point the panel's "selected row" field at whatever the chain has focused, so the engine's own +/- handling acts on the row the user is actually on.
- L94 `void CaptureLabels(const PanelDesc& d, void* panel)`: Bind each row's NAME label to its value button in the cycle-category cache.
- L106 `bool AnnounceDescription(const PanelDesc& d, void* panel, void* control)`: Speak the focused row's description.
- L110 `bool IsDescriptionListbox(const PanelDesc& d, void* listBox)`: True iff `listBox` is this panel's description listbox.
