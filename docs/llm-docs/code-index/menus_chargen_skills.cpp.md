# menus_chargen_skills.cpp (151 lines)

Source: `patches/Accessibility/menus_chargen_skills.cpp`

chargen Skills panel tweaks. See menus_chargen_skills.h for the design rationale.

## Declarations (in source order)

- L38 `bool IsChargenSkillsPanel(void* panel)`
- L42 `int SkillIndexFromButton(void* panel, void* control)`
- L46 `void SyncSelectedSkillFromChainFocus()`
- L50 `void CaptureLabelsIfApplicable(void* panel)`
- L54 `int RowPitchForCursorWarp(void* panel, void* control)`
- L67 `int ReadEngineSkillCost(void* panel, int skillIdx)`
- L89 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L114 `bool AnnounceChainStepDescription(void* panel, void* control)`
- L118 `bool IsChargenSkillsDescriptionListbox(void* listBox)`
- L122 `bool AnnounceValueChange(void* panel, void* control)`
