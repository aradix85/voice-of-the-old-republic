# menus_focus.h (34 lines)

Source: `patches/Accessibility/menus_focus.h`

First-sight / focus-capture seam.

Everything here is driven by OnSetActiveControl (menus_setactive.cpp), which fires on every real focus change. The functions split that hook's work into the four things it does on a focus event:

One exception: RebindChain also calls WalkAndCaptureOnFirstSight, because a panel can become the user's foreground WITHOUT the engine ever firing SetActiveControl on it — see the comment at that call site.

* PrefillClassIconCacheOnTransition — pump the chargen class-label cache with the OUTGOING icon before the engine overwrites it. * UpdateFocusedPanelState — publish the new panel as g_currentPanel. * WalkAndCaptureOnFirstSight — one-shot per-panel child dump + cycle-category capture. * SpeakPanelTitleOnFirstSight — the entry banner ("<panel title>"). * AnnounceNewFocusedControl — queue the focused control into the pending-announce slot for the next tick's drain.

AnnouncePanelTitle stays file-static in menus_focus.cpp: its only caller is SpeakPanelTitleOnFirstSight, which moved with it.

## Declarations (in source order)

- L27 `void PrefillClassIconCacheOnTransition(void* panel, void* newControl)`
- L28 `void UpdateFocusedPanelState(void* panel)`
- L29 `void WalkAndCaptureOnFirstSight(void* panel)`
- L30 `void SpeakPanelTitleOnFirstSight(void* panel)`
- L31 `void AnnounceNewFocusedControl(int n, void* panel, void* newControl)`
