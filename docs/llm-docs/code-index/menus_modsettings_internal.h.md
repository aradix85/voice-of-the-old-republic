# menus_modsettings_internal.h (113 lines)

Source: `patches/Accessibility/menus_modsettings_internal.h`

Internal to the menus_modsettings module (menus_modsettings*.cpp); no other file includes it. The public interface is menus_modsettings.h. Here: what the files share.

## Declarations (in source order)

- L13 `using Id`
- L15 `enum class RowKind`
- L22 `struct OptionSpec`
- L47 `static_assert(k_optionCount == static_cast<int>(Option::Count), "")`
- L50 `constexpr bool OptionsInEnumOrder()`
- L56 `static_assert(OptionsInEnumOrder(), "")`
- L75 `static_assert(k_rootRowCount == k_optionCount, "")`
- L84 `extern bool s_toggles[static_cast<int>(Option::Count)]`: menus_modsettings.cpp
- L97 `const OptionSpec& Spec(Option opt)`: menus_modsettings.cpp
- L98 `Option FocusedOption()`
- L99 `void SpeakFocusedOption(bool interrupt = true)`
- L101 `bool StepListCursor(int keyCode, int count, int* cursor)`: Up/Down/Home/End on a list of `count` rows, clamped; false for any other key.
- L104 `void OpenGlossarySubMenu()`: menus_modsettings_glossary.cpp
- L105 `void ResetGlossary()`
- L106 `bool HandleInputGlossary(int keyCode)`
- L107 `void TickGlossary()`
- L110 `bool HandleInputList(int keyCode)`: menus_modsettings_list.cpp

Also 9 constants or variables at namespace scope.
