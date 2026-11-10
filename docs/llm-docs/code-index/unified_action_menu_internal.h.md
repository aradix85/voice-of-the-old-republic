# unified_action_menu_internal.h (141 lines)

Source: `patches/Accessibility/unified_action_menu_internal.h`

Internal to the unified_action_menu module (unified_action_menu*.cpp); no other file includes it. The public interface is unified_action_menu.h. Here: what the files share.

## Declarations (in source order)

- L20 `enum class CatKind`
- L22 `struct Cat`
- L38 `struct MedPicker`: KOTOR 2 medical-item party picker — the keyboard's version of the portrait click that completes K2's two-step medical use (see PrimeMedicalPickK2).
- L56 `struct State`
- L104 `int ClampInt(int v, int lo, int hi)`: unified_action_menu.cpp
- L105 `bool ForegroundPanelBlocks()`
- L106 `int CountForCat(void* tam, void* mi, const Cat& c)`
- L107 `int& ShadowFor(const Cat& c)`
- L108 `void ApplySelection(void* tam, void* mi, const Cat& c, int idx)`
- L109 `void ReadLabel(void* tam, void* mi, const Cat& c, int idx, char* out, size_t n)`
- L110 `uint32_t ReadActionId(void* tam, void* mi, const Cat& c, int idx)`
- L111 `bool Dispatch(void* tam, void* mi, const Cat& c)`
- L112 `const char* CategoryName(const Cat& c)`
- L113 `void BuildCategoryList(void* tam, void* mi)`
- L114 `int LocateCat(CatKind kind, int slot)`
- L115 `int FirstPopulatedTargetRow(void* tam)`
- L116 `bool DetectCreature(void* tam, uint32_t handle)`
- L117 `uint32_t ResolveNarratedServerHandle()`
- L118 `void Arm()`
- L121 `void SpeakCategory(void* tam, void* mi, const char* prefix)`: unified_action_menu_speech.cpp
- L122 `void SpeakEntry(void* tam, void* mi)`
- L123 `void DescribeSelectedEntry(void* tam, void* mi, const Cat& cur, int sel)`
- L126 `void CloseFromEsc()`: unified_action_menu_open.cpp
- L127 `bool ReanchorToNewTarget(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`
- L128 `bool UnfoldTargetBlock(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`
- L129 `void MaybeRestoreDrainedRows(void*& tam, bool targetChanged)`
- L132 `void ApplyPostFireClosePolicy()`: unified_action_menu_fire.cpp
- L133 `bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel, bool targetChanged, bool selectionCarried)`
- L136 `bool FireMedicalDirect(void* mi, const Cat& c, int sel, void* targetCre, const char* how)`: unified_action_menu_medpicker.cpp
- L137 `bool OpenMedPicker(void* mi, const Cat& c, int sel, const char* itemLabel)`
- L138 `bool HandleMedPickerInput(int code)`

Also 6 constants or variables at namespace scope.
