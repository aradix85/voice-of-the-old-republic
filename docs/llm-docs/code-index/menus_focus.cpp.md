# menus_focus.cpp (519 lines)

Source: `patches/Accessibility/menus_focus.cpp`

First-sight / focus-capture logic for the panel focus hook.

Split out of menus.cpp by the Phase-1 structure pass (refactoring candidate 1). OnSetActiveControl stays in menus.cpp and calls into these five functions; see menus_focus.h for what each one owns.

The definitions are verbatim from menus.cpp apart from being lifted out of file-static scope into acc::menus::focus so the hook can still reach them. g_lastTitledPanel moved with SpeakPanelTitleOnFirstSight (its only user); the pending-announce slot stays owned by menus.cpp and is written here through the externs declared in menus_internal.h.

## Declarations (in source order)

- L69 `static void AnnouncePanelTitle(void* panel)`: First focus into a panel speaks the panel's "title" — the first label-like child we can find — so the user knows which menu they're in.
- L289 `void acc::menus::focus::PrefillClassIconCacheOnTransition(void* panel, void* newControl)`: Chargen class-icon cache pump.
- L325 `void acc::menus::focus::UpdateFocusedPanelState(void* panel)`: Update g_currentPanel on panel transitions.
- L336 `void acc::menus::focus::WalkAndCaptureOnFirstSight(void* panel)`: First focus event into a previously-unseen panel: dump every child control + capture cycle-button category text.
- L424 `void acc::menus::focus::SpeakPanelTitleOnFirstSight(void* panel)`: First focus into a new panel: speak its title once.
- L449 `void acc::menus::focus::AnnounceNewFocusedControl(int n, void* panel, void* newControl)`: Record the new focused control into the pending-announce slot for the drain to read on the next tick.
