# menus_focus.cpp (512 lines)

Source: `patches/Accessibility/menus_focus.cpp`

First-sight / focus-capture logic for the panel focus hook: OnSetActiveControl (menus_setactive.cpp) calls these five functions; see menus_focus.h for what each one owns. The pending-announce slot is menus.cpp's, written here through the externs in menus_internal.h.

## Declarations (in source order)

- L62 `static void AnnouncePanelTitle(void* panel)`: First focus into a panel speaks the panel's "title" — the first label-like child we can find — so the user knows which menu they're in.
- L282 `void acc::menus::focus::PrefillClassIconCacheOnTransition(void* panel, void* newControl)`: Chargen class-icon cache pump.
- L318 `void acc::menus::focus::UpdateFocusedPanelState(void* panel)`: Update g_currentPanel on panel transitions.
- L329 `void acc::menus::focus::WalkAndCaptureOnFirstSight(void* panel)`: First focus event into a previously-unseen panel: dump every child control + capture cycle-button category text.
- L417 `void acc::menus::focus::SpeakPanelTitleOnFirstSight(void* panel)`: First focus into a new panel: speak its title once.
- L442 `void acc::menus::focus::AnnounceNewFocusedControl(int n, void* panel, void* newControl)`: Record the new focused control into the pending-announce slot for the drain to read on the next tick.
