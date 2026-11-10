# unified_action_menu_internal.h (139 lines)

Source: `patches/Accessibility/unified_action_menu_internal.h`

Internal to the unified_action_menu module (unified_action_menu*.cpp); no other file includes it. The public interface is unified_action_menu.h. Here: what the files share.

## Declarations (in source order)

- L21 `enum class CatKind`
- L23 `struct Cat`
- L39 `struct MedPicker`: KOTOR 2 medical-item party picker — the keyboard's version of the portrait click that completes K2's two-step medical use (see PrimeMedicalPickK2).
- L56 `struct State`
- L101 `int ClampInt(int v, int lo, int hi)`: unified_action_menu.cpp
- L102 `bool ForegroundPanelBlocks()`
- L103 `int CountForCat(void* tam, void* mi, const Cat& c)`
- L104 `int& ShadowFor(const Cat& c)`
- L105 `void ApplySelection(void* tam, void* mi, const Cat& c, int idx)`
- L106 `void ReadLabel(void* tam, void* mi, const Cat& c, int idx, char* out, size_t n)`
- L107 `uint32_t ReadActionId(void* tam, void* mi, const Cat& c, int idx)`
- L108 `bool Dispatch(void* tam, void* mi, const Cat& c)`
- L109 `const char* CategoryName(const Cat& c)`
- L110 `void FormatEmptyColumn(int col, char* out, size_t n)`
- L111 `void BuildCategoryList(void* tam, void* mi)`
- L112 `int LocateCat(CatKind kind, int slot)`
- L113 `int FirstPopulatedTargetRow(void* tam)`
- L114 `bool DetectCreature(void* tam, uint32_t handle)`
- L115 `uint32_t ResolveNarratedServerHandle()`
- L116 `void Arm()`
- L119 `void SpeakCategory(void* tam, void* mi, const char* prefix)`: unified_action_menu_speech.cpp
- L120 `void SpeakEntry(void* tam, void* mi)`
- L121 `void DescribeSelectedEntry(void* tam, void* mi, const Cat& cur, int sel)`
- L124 `void CloseFromEsc()`: unified_action_menu_open.cpp
- L125 `bool ReanchorToNewTarget(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`
- L126 `bool UnfoldTargetBlock(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`
- L127 `void MaybeRestoreDrainedRows(void*& tam, bool targetChanged)`
- L130 `void ApplyPostFireClosePolicy()`: unified_action_menu_fire.cpp
- L131 `bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel, bool targetChanged, bool selectionCarried)`
- L134 `bool FireMedicalDirect(void* mi, const Cat& c, int sel, void* targetCre, const char* how)`: unified_action_menu_medpicker.cpp
- L135 `bool OpenMedPicker(const Cat& c, int sel, const char* itemLabel)`
- L136 `bool HandleMedPickerInput(int code)`

Also 6 constants or variables at namespace scope.
