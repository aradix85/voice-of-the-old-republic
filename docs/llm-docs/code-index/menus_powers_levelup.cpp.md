# menus_powers_levelup.cpp (97 lines)

Source: `patches/Accessibility/menus_powers_levelup.cpp`

The Force-power chart: what menus_skillflow_grid needs to know about CSWGuiPowersLevelUp.

## Declarations (in source order)

- L23 `void* Chart(void* panel)`
- L31 `bool ReadRows(void* panel, void** rows, int* count)`: The rows are the controls of the powers list box.
- L44 `void OnEnterPower(void* panel, unsigned long powerId)`
- L48 `void OnPowerPicked(void* panel, unsigned long powerId)`
- L78 `bool IsPowersLevelUpPanel(void* panel)`
- L82 `const char* GetTitleOverride(void* panel)`
- L92 `bool HandleInput(void* panel, int code, int value, int& outRv)`
