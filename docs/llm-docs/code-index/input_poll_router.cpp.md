# input_poll_router.cpp (347 lines)

Source: `patches/Accessibility/input_poll_router.cpp`

Per-tick Win32 hotkey router - see input_poll_router.h for what this is and why it is not merged into input_pipeline.cpp.

The gate ORDER in PollHotkey is behaviour. Do not reorder without an in-game input pass.

## Declarations (in source order)

- L56 `bool NumberOpener() const`
- L63 `Edges ReadEdges()`
- L92 `void Say(Id id)`
- L96 `void OpenLevelUp()`: Shift+L opens the engine's level-up panel directly (CGuiInGame::ShowLevelUpGUI), from any screen.
- L121 `void HandleOpeners(const Edges& e)`: The Shift+number openers of the action menu, and Shift+L.
- L150 `void AnnounceBareKeys(const Edges& e)`: Bare 1..7: the engine fires the action through its own handler whatever we do; this only speaks what was fired.
- L167 `void RouteBasicKeys(const Edges& e, Handler handle)`
- L179 `bool RouteToOverlay(const Edges& e)`: True when an armed overlay owned this tick's keys.
- L206 `void AnnounceNativeDefaultAction()`: Bare R: the engine's "default action on current target" fires on its own; this adds the spoken pre-roll from the live descriptor, read-only.
- L242 `bool EnterMayActInWorld(const char* keyTag)`: True when no blocking UI panel is in front, so Enter may act in the world.
- L284 `void HandleWorldEnter(bool forceRadial)`: Enter and Shift+Enter in the world: interact with the narrated target.
- L302 `void PollHotkey()`
