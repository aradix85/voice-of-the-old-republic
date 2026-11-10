# unified_action_menu_medpicker.cpp (197 lines)

Source: `patches/Accessibility/unified_action_menu_medpicker.cpp`

KOTOR 2 medical items: the party picker that chooses who gets the medpac or stim.

## Declarations (in source order)

- L32 `void SpeakMedPickerRow(bool withTitle)`
- L51 `bool FireMedicalDirect(void* mi, const Cat& c, int sel, void* targetCre, const char* how)`: Fire a Medicine-row entry at a specific CLIENT creature via the direct use-item request (DoPersonalAction is a dead end for K2 medical items — see SendUseItemRequestK2).
- L76 `bool OpenMedPicker(void* mi, const Cat& c, int sel, const char* itemLabel)`: Returns true when the picker opened (the press is consumed).
- L142 `bool HandleMedPickerInput(int code)`
