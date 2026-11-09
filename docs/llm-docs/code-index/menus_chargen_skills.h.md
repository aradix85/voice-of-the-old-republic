# menus_chargen_skills.h (106 lines)

Source: `patches/Accessibility/menus_chargen_skills.h`

chargen "Fähigkeiten" panel (step 3 of Eigener Charakter) tweaks.

Mirror of menus_chargen_attr.{h,cpp} for CSWGuiSkillsCharGen — the engine struct shape is the same: 8 skill_buttons + 8 skill_labels + 8 plus_buttons + 8 minus_buttons + a panel-level selected_skill_index that the engine's shared OnPlusButton / OnMinusButton handlers read to pick which skill changes. Same fixes apply:

1. Mirror chain focus → selected_skill_index so Left/Right modifies the focused row instead of a default top row. 2. Capture skill_buttons[i] → skill_labels[i] text into the cycle-category cache so FromControl announces "Computer, 0" instead of bare "0". 3. Compensate the cursor-warp Y by one row pitch so the engine's hit-test resolves to the right row (same shift Options' tab cluster needs g_tabClickOffsetY for, and that the Attribute panel needs RowPitchForCursorWarp for).

Differences from the Attribute panel:

* 8 skills instead of 6 abilities; skill order is straight top-to-bottom (no struct vs visual swap). * Skills don't have a D&D modifier — only rank + cost-per-+1. * Cost is constant per skill (1 for class, 2 for cross-class) throughout the row's normal range. We compute it ourselves via the engine's CSWGuiSkillsCharGen::IsClassSkill predicate instead of reading the cost_value label (same hit-test / refresh-timing race we fought through on the Attribute panel), and the value-change announce never repeats it (the user heard it in the chain-step suffix).

## Declarations (in source order)

- L39 `bool IsChargenSkillsPanel(void* panel)`: True iff `panel`'s vtable matches CSWGuiSkillsCharGen.
- L45 `int SkillIndexFromButton(void* panel, void* control)`: Index 0..7 in struct order if `control` is one of the panel's skill_buttons[]; -1 otherwise.
- L54 `void SyncSelectedSkillFromChainFocus()`: Mirror chain focus into the panel's selected_skill_index so the next +/- press routes OnPlusButton / OnMinusButton to the focused skill rather than the default top row.
- L58 `void CaptureLabelsIfApplicable(void* panel)`: Capture each skill_button → its matching skill_label text into the cycle-category cache, so FromControl produces "Computer, 0" etc.
- L64 `int RowPitchForCursorWarp(void* panel, void* control)`: Cursor-Y compensation for the chain-step click-sim when focus is a skill_button.
- L72 `void AnnounceChainStepSuffix(void* panel, void* control)`: Speak the per-row info suffix ("Preis 1" / "Preis 2") synchronously from the chain-step input handler, right after the existing "Computer, 0" announce.
- L86 `bool AnnounceChainStepDescription(void* panel, void* control)`: Speak the focused skill's description text by reading skill_descriptions[i] directly from the panel struct, bypassing the engine's hover-driven listbox population.
- L92 `bool IsChargenSkillsDescriptionListbox(void* listBox)`: True iff `listBox` is the description_list_box of a chargen Skills panel currently in the chain.
- L103 `bool AnnounceValueChange(void* panel, void* control)`: On a +/- press, override the default "{label}, {value}" re-announcement with "{value}, verbleibende Punkte {remaining}".
