# menus.cpp (351 lines)

Source: `patches/Accessibility/menus.cpp`

The menu side's own state and per-tick glue: the speech dedup channels, the focused panel, the sub-screen drill flag, the pending-announce slot the panel SetActiveControl hook feeds (menus_setactive.cpp) and its drain, and what core_tick calls each frame (ValidatePanels, TickMonitors, PollHomeEndKeys, TickPendingOps).

## Declarations (in source order)

- L73 `void MarkSpoken(int channel, const char* text)`
- L79 `void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs)`
- L93 `bool AlreadySpokenOnAnyChannel(const char* text)`
- L107 `bool acc::menus::detail::IsMultiRowListBox(void* control)`
- L121 `void* CurrentPanelIfLive()`: See menus_internal.h for why every deref of g_currentPanel has to come through here.
- L177 `void ValidatePanels()`
- L193 `void TickMonitors()`
- L212 `void PollHomeEndKeys()`
- L243 `void TickPendingOps()`: Drain the menu-side pending-op queue (menus_pending).
- L266 `void DrainPendingAnnounce()`: Drain the pending-announce slot.
- L340 `void ClearPendingAnnounce()`
- L345 `bool IsDrilledIntoSubScreen()`
- L346 `void SetDrilledIntoSubScreen(bool drilled)`
