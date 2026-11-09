# menus_modsettings.h (177 lines)

Source: `patches/Accessibility/menus_modsettings.h`

virtual mod-settings submenu.

Layer: menus/ (UI state + speech). Reachable via a virtual "Mod Einstellungen" chain entry that we inject into both the in-game Optionen panel (PanelKind::InGameOptions) and the title-screen Optionen panel (PanelKind::MainMenuOptions).

Design summary:

* The chain entry is NOT backed by a real engine control. We use a static sentinel pointer (see GetRootAnchor) so we never hijack someone else's button and never touch engine-allocated controls. RebindChain inserts a virtual ChainEntry whose `virtualKind == kVirtualMod_SettingsRoot`; menus_extract::FromControl special- cases the sentinel and returns the localised "Mod Einstellungen" phrase; the Enter dispatcher in menus.cpp special-cases the virtual-kind tag and opens the submenu instead of FireActivate.

* The submenu itself is entirely virtual — there is no second engine panel. While IsOpen() is true, the manager input hook routes navigation keys through HandleInput before the normal chain dispatcher sees them. Up/Down cycles the focused option, Space/Enter toggles, Esc closes and rebinds the parent chain.

* Rows can live one level deeper. The root list and the announcement-voice submenu are two views over the same Option enum and share one input handler; Esc inside the nested view returns to the root rather than to the parent panel. The audio glossary and the keybind configurator are separate views with their own row model.

* Three dummy toggle options ship as scaffolding: extended cycling, room shape descriptions, wall sounds. The toggle bits live in this TU (in-memory only — persistence is a follow-up). Real consumers will read GetToggle(option) once those features are wired.

## Declarations (in source order)

- L60 `enum class Option`: Per-option identifiers.
- L110 `void* GetRootAnchor()`: Sentinel pointer used as the `control` field of the virtual chain entry.
- L116 `bool IsRootAnchor(void* control)`: True iff `control` is the mod-settings root sentinel.
- L124 `void ForEachRootAnchor(void* panel, bool (*callback)(void* sentinel, int sortCx, int sortCy, void* userData), void* userData)`: Visit the registered virtual chain anchor for `panel`.
- L132 `bool ExtractRootLabel(char* outBuf, size_t bufSize)`: Format the root entry's speech ("Mod Einstellungen").
- L136 `void OpenSubMenu(void* parentPanel)`: Open the submenu.
- L141 `bool IsOpen()`: True while the submenu is interactive.
- L146 `void Close()`: Close the submenu and rebind the chain on the saved parent panel.
- L160 `bool HandleInput(int keyCode)`: Input-press handler.
- L167 `bool GetToggle(Option option)`: Read the current state of a toggle option.
- L174 `void Tick()`: Per-tick poll.
