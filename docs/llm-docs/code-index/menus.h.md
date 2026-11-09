# menus.h (70 lines)

Source: `patches/Accessibility/menus.h`

Public surface of menus.cpp — entry points core_tick calls.

## Declarations (in source order)

- L9 `void ValidatePanels()`: First per tick — drop pointers the engine may have freed (currently the tabbed-panel cluster pointer).
- L13 `void TickMonitors()`: Focus-text / panel-contents / dialog-reply / container-loot / equip-picker selection deltas + container give-mode G-key poll.
- L17 `void PollHomeEndKeys()`: Home/End rising edges.
- L20 `void TickPendingOps()`: LAST per tick so no monitor sees a partially-applied state.
- L26 `void DrainPendingAnnounce()`: OnSetActiveControl writes the slot per panel-focus event; this drains it once.
- L31 `void ClearPendingAnnounce()`: Drops the slot without speaking — for subsystems that announced focus in their own format (e.g.
- L47 `void SpeakIfChanged(int channel, const char* text, unsigned repeatAfterMs = 0)`: Channel-keyed dedup shared by panel-focus drain (ch 0) and listbox-row hook (ch 1).
- L49 `void MarkSpoken(int channel, const char* text)`
- L58 `bool AlreadySpokenOnAnyChannel(const char* text)`: True when `text` is what was last spoken on EITHER channel.
- L66 `bool IsDrilledIntoSubScreen()`: g_drilledIntoSubScreen retargets arrow-key nav from the InGameMenu strip (engine's strip-stays-fg pattern) to the visible sub-screen.
- L67 `void SetDrilledIntoSubScreen(bool drilled)`
