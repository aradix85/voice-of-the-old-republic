# menus.cpp (673 lines)

Source: `patches/Accessibility/menus.cpp`

The menu side's own state and hooks: the speech dedup channels, the focused panel, the sub-screen drill flag, the two SetActiveControl hooks (panel and listbox) with the pending-announce slot they feed, and the per-tick glue core_tick calls (ValidatePanels, TickMonitors, PollHomeEndKeys, TickPendingOps).

## Declarations (in source order)

- L85 `bool InMovieBurstWindow()`: True while the engine-GUI SetActive rebuild burst must stay silent because a cutscene movie is on screen — or was, within the last kMovieBurstTailMs.
- L106 `const char* BurstSuppression()`: Why both SetActiveControl hooks stay silent right now, or nullptr.
- L114 `int ListBoxRowCount(void* listBox)`
- L148 `void MarkSpoken(int channel, const char* text)`
- L154 `void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs)`
- L168 `bool AlreadySpokenOnAnyChannel(const char* text)`
- L182 `bool acc::menus::detail::IsMultiRowListBox(void* control)`
- L196 `void* CurrentPanelIfLive()`: See menus_internal.h for why every deref of g_currentPanel has to come through here.
- L262 `extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl)`: CSWGuiPanel::SetActiveControl — hooked mid-function at 0x0040a638.
- L344 `extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow, int param2)`: CSWGuiListBox::SetActiveControl — hooked mid-function at 0x0041c16b.
- L499 `void ValidatePanels()`
- L515 `void TickMonitors()`
- L534 `void PollHomeEndKeys()`
- L565 `void TickPendingOps()`: Drain the menu-side pending-op queue (menus_pending).
- L588 `void DrainPendingAnnounce()`: Drain the pending-announce slot.
- L662 `void ClearPendingAnnounce()`
- L667 `bool IsDrilledIntoSubScreen()`
- L668 `void SetDrilledIntoSubScreen(bool drilled)`
