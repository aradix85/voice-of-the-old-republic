# menus.cpp (231 lines)

Source: `patches/Accessibility/menus.cpp`

The menu side's own state and per-tick glue: the speech dedup channels, the focused panel, the sub-screen drill flag, the pending-announce slot the panel SetActiveControl hook feeds (menus_setactive.cpp) and its drain, and what core_tick calls each frame (ValidatePanels, TickMonitors, PollHomeEndKeys, TickPendingOps).

## Declarations (in source order)

- L42 `void* CurrentPanelIfLive()`: IsPanelInManager compares pointers only, so it is safe on a freed panel.
- L67 `bool SameAsLast(int channel, const char* text)`
- L71 `void Remember(int channel, const char* text)`
- L89 `void MarkSpoken(int channel, const char* text)`
- L97 `void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs)`: Queued, never interrupting: focus events fire about ten times a second while a panel builds, and interrupting cut every line mid-word.
- L107 `bool AlreadySpokenOnAnyChannel(const char* text)`
- L111 `bool detail::IsMultiRowListBox(void* control)`
- L115 `void ValidatePanels()`
- L123 `void TickMonitors()`
- L139 `void PollHomeEndKeys()`: The engine drops KEYBOARD_HOME (32) and KEYBOARD_END (33) before the manager hook (no keymap action targets them), so they are polled here and fed through the hook as if the engine had routed them.
- L157 `void TickPendingOps()`: After all monitors, which keeps deep engine re-entry off the input hook's stack.
- L168 `bool IsChainSiblingEcho(void* panel, void* control)`: After a chain step the engine can echo a SetActive on a sibling chain control (the InGameOptions hit-test miss: the chain steps to Gameplay, the engine activates Load Game).
- L185 `void DrainPendingAnnounce()`
- L222 `void ClearPendingAnnounce()`
- L227 `bool IsDrilledIntoSubScreen()`
- L228 `void SetDrilledIntoSubScreen(bool drilled)`
