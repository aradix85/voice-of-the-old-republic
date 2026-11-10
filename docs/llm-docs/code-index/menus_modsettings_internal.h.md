# menus_modsettings_internal.h (162 lines)

Source: `patches/Accessibility/menus_modsettings_internal.h`

Internal to the menus_modsettings module (menus_modsettings*.cpp); no other file includes it. The public interface is menus_modsettings.h. Here: what the files share.

## Declarations (in source order)

- L34 `extern bool s_toggles[static_cast<int>(Option::Count)]`: Toggle state.
- L85 `enum class RowKind`
- L92 `struct OptionSpec`
- L117 `static_assert(k_optionCount == static_cast<int>(Option::Count), "")`
- L142 `static_assert(k_rootRowCount == k_optionCount, "")`: Every option is a row of the list: a row added to the enum but not here would be invisible with nothing else to catch it.
- L148 `const OptionSpec& Spec(Option opt)`: menus_modsettings.cpp
- L149 `Option FocusedOption()`
- L150 `const char* PersistKey(Option opt)`
- L151 `void SpeakFocusedOption(bool interrupt = true)`
- L154 `void CancelPendingGlossaryCue()`: menus_modsettings_glossary.cpp
- L155 `void OpenGlossarySubMenu()`
- L156 `bool HandleInputGlossary(int keyCode)`
- L159 `bool HandleInputList(int keyCode)`: menus_modsettings_list.cpp

Also 16 constants or variables at namespace scope.
