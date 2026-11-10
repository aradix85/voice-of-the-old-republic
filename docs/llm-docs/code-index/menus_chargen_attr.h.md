# menus_chargen_attr.h (126 lines)

Source: `patches/Accessibility/menus_chargen_attr.h`

chargen attributes panel (step 2 of Custom Character) tweaks.

Two fixes for CSWGuiAbilitiesCharGen, both rooted in the same engine quirk: the panel's six +/- buttons all dispatch to OnPlusButton / OnMinusButton, which read selected_ability — they do NOT inspect the fired button's identity. So:

1. Each value button's chain-announce is just its number ("8") with no ability name, leaving the user blind to which row they're on. We mirror the matching ability_label's text into the cycle- category cache; FromControl's existing prefix logic then says "Strength, 8" instead of bare "8".

2. selected_ability stays at 0 (STR) for the panel's lifetime because the engine only writes it on a real mouse click — our chain-nav cursor warp doesn't trigger OnEnterPointsButton reliably (hit-test resolves one row above the cursor here too). So every Left/Right press, regardless of focused row, increments STR. We sync chain focus → selected_ability on every chain rebind and Up/Down step so +/- targets the row the user is on.

Engine offsets in engine_offsets.h. Caller-side wiring in menus.cpp (CaptureLabelsIfApplicable from WalkAndCaptureOnFirstSight, sync from chain step) and menus_chain.cpp (sync from end of RebindChain).

## Declarations (in source order)

- L32 `bool IsChargenAttributesPanel(void* panel)`: True iff `panel`'s vtable matches CSWGuiAbilitiesCharGen.
- L37 `int AbilityIndexFromButton(void* panel, void* control)`: Index 0..5 in struct order if `control` is one of the panel's ability_buttons[]; -1 otherwise.
- L48 `int RowPitchForCursorWarp(void* panel, void* control)`: Cursor-Y compensation for the chain-step click-sim when the focus is one of the ability_buttons[].
- L69 `void SyncSelectedAbilityFromChainFocus()`: Write selected_ability for the chain's currently-focused button.
- L74 `void CaptureLabelsIfApplicable(void* panel)`: Capture each ability_button → its matching ability_label text into the cycle-category cache, so FromControl produces "Strength, 8" etc.
- L96 `void AnnounceChainStepSuffix(void* panel, void* control)`: Speak the per-row info suffix ("Modifier -1, Cost 1") synchronously from the chain-step input handler, right after the existing "Strength, 8" announce.
- L105 `bool AnnounceChainStepDescription(void* panel, void* control)`: Speak the focused ability's description by calling the engine's OnEnterPointsButton directly with the focused button (keyed by control, not by a resolution-dependent cursor-warp hit-test) and reading the resulting des...
- L110 `bool IsChargenAttributesDescriptionListbox(void* listBox)`: True iff `listBox` is this panel's description_listbox.
- L123 `bool AnnounceValueChange(void* panel, void* control)`: On a +/- press, override the default "{label}, {value}" re-announcement with "{value}, remaining points {remaining}".
