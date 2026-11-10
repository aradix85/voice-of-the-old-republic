# menus_chain_input_keys.cpp (164 lines)

Source: `patches/Accessibility/menus_chain_input_keys.cpp`

Left/Right on the chain cursor (sliders, cycle arrows) and Esc (back out of popups and sub-screens).

## Declarations (in source order)

- L26 `void* PortraitArrowTarget(void* panel, void* focused, bool toRight)`: Character creation's portrait picker keeps only its left arrow in the chain, and nothing lies left of it for FindAdjacentArrow: on that arrow, Left and Right activate the left and right arrow directly.
- L38 `bool EscWorkbenchUpgrade(void* panel)`: The upgrade screen is the foreground panel, not a popup, so the generic Esc below does not see it: Esc goes to BTN_BACK.
- L56 `void EscPopup(void* panel, bool isOptionsSub, bool& consumed)`: Esc on a popup, a tabbed sub-dialog or an in-game Options sub-screen: activate its Cancel, or else its Close/OK/Continue.
- L83 `void HandleLeftRight(void* activePanel, int code, int val, bool& consumed)`
- L130 `void HandleEsc(void* activePanel, int code, int val, bool& consumed)`
