# menus_focus.cpp (423 lines)

Source: `patches/Accessibility/menus_focus.cpp`

First-sight / focus-capture logic for the panel focus hook: OnSetActiveControl (menus_setactive.cpp) calls these five functions; see menus_focus.h for what each one owns. The pending-announce slot is menus.cpp's, written here through the externs in menus_internal.h.

## Declarations (in source order)

- L55 `const char* TitleOverride(void* panel, const char** source)`: A title one of the screen modules supplies itself, where the first label of the .gui file is wrong (a placeholder, or the header of another screen).
- L83 `void WakeInputAtMainMenu()`: On a fresh launch the engine reaches the main menu with its DirectInput keyboard unacquired: menu input is dead until an alt-tab forces a SetActive(0) -> (1) edge.
- L97 `const char* NotATitle(const char* text)`: Why a label's text cannot be a panel title, or nullptr when it can.
- L112 `void SpeakFirstLabelAsTitle(void* panel)`: The panel's first label with text that can be a title.
- L150 `static void AnnouncePanelTitle(void* panel)`: The first focus into a panel speaks its title, so that the user knows which menu this is.
- L194 `void acc::menus::focus::PrefillClassIconCacheOnTransition(void* panel, void* newControl)`: Chargen class-icon cache pump.
- L230 `void acc::menus::focus::UpdateFocusedPanelState(void* panel)`: Update g_currentPanel on panel transitions.
- L241 `void acc::menus::focus::WalkAndCaptureOnFirstSight(void* panel)`: First focus event into a previously-unseen panel: dump every child control + capture cycle-button category text.
- L329 `void acc::menus::focus::SpeakPanelTitleOnFirstSight(void* panel)`: First focus into a new panel: speak its title once.
- L354 `void acc::menus::focus::AnnounceNewFocusedControl(int n, void* panel, void* newControl)`: Record the new focused control into the pending-announce slot for the drain to read on the next tick.
