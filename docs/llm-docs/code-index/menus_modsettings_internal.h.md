# menus_modsettings_internal.h (111 lines)

Source: `patches/Accessibility/menus_modsettings_internal.h`

Internal to the menus_modsettings module (menus_modsettings*.cpp); no other file includes it. The public interface is menus_modsettings.h. Here: what the files share.

## Declarations (in source order)

- L13 `using Id`
- L15 `enum class RowKind`
- L22 `struct OptionSpec`
- L46 `static_assert(k_optionCount == static_cast<int>(Option::Count), "")`
- L49 `constexpr bool OptionsInEnumOrder()`
- L55 `static_assert(OptionsInEnumOrder(), "")`
- L73 `static_assert(k_rootRowCount == k_optionCount, "")`
- L82 `extern bool s_toggles[static_cast<int>(Option::Count)]`: menus_modsettings.cpp
- L95 `const OptionSpec& Spec(Option opt)`: menus_modsettings.cpp
- L96 `Option FocusedOption()`
- L97 `void SpeakFocusedOption(bool interrupt = true)`
- L99 `bool StepListCursor(int keyCode, int count, int* cursor)`: Up/Down/Home/End on a list of `count` rows, clamped; false for any other key.
- L102 `void OpenGlossarySubMenu()`: menus_modsettings_glossary.cpp
- L103 `void ResetGlossary()`
- L104 `bool HandleInputGlossary(int keyCode)`
- L105 `void TickGlossary()`
- L108 `bool HandleInputList(int keyCode)`: menus_modsettings_list.cpp

Also 9 constants or variables at namespace scope.
