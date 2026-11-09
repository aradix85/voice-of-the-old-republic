# menus_chargen_attr.cpp (286 lines)

Source: `patches/Accessibility/menus_chargen_attr.cpp`

chargen attributes panel tweaks. See menus_chargen_attr.h for the design rationale.

## Declarations (in source order)

- L37 `bool IsChargenAttributesPanel(void* panel)`
- L41 `int AbilityIndexFromButton(void* panel, void* control)`
- L45 `void SyncSelectedAbilityFromChainFocus()`
- L49 `int RowPitchForCursorWarp(void* panel, void* control)`
- L53 `void CaptureLabelsIfApplicable(void* panel)`
- L63 `int ParseAbilityValueText(const char* text)`: Parse the engine-rendered ability value text ("8", "10", "18") into an int.
- L83 `int ComputeAbilityModifier(int value)`: D&D 3.5 ability modifier: floor((value - 10) / 2).
- L98 `int ReadEngineAbilityCost(void* panel, int currentValue)`
- L120 `void FormatModifier(int mod, char* outBuf, size_t bufSize)`: Render the modifier with sign so it reads correctly through NVDA: positive gets an explicit "+", zero is bare "0" (engine renders this as bare "-" which sounds broken), negative carries its own sign already.
- L132 `bool AnnounceChainStepDescription(void* panel, void* control)`
- L136 `bool IsChargenAttributesDescriptionListbox(void* listBox)`
- L140 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L189 `void ResetTrackerIfPanelChanged(void* panel)`
- L199 `bool AnnounceValueChange(void* panel, void* control)`
