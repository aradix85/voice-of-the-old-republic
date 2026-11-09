# menus_galaxymap.h (46 lines)

Source: `patches/Accessibility/menus_galaxymap.h`

Galaxy / star-map travel screen accessibility (CSWGuiInGameGalaxyMap, galaxymap.gui — opened from the Ebon Hawk navigation console).

The panel is a single-axis "cycle one planet, then Travel / Cancel" surface. Its planet hotspots are image-only CSWGuiButtons with empty captions (they render as "control N" through the generic chain), and the real selection state lives server-side on CSWPartyTable, NOT on the panel — so the generic chain can neither name the planets nor tell which are hidden.

Instead we drive the engine's own CSWGuiInGameGalaxyMap::HandleInputEvent, whose NextPlanet/PrevPlanet handlers already skip planets the party table reports as unavailable (not yet revealed) or unselectable (can't travel there now). Up/Down cycle the reachable planets and announce each name from LBL_PLANETNAME; Enter travels (accept), Esc cancels (back); Shift+Down reads LBL_DESC (handled via peek_description). First sight speaks a fixed title plus the current planet name.

## Declarations (in source order)

- L23 `bool IsGalaxyMapPanel(void* panel)`: IdentifyPanel(panel) == PanelKind::InGameGalaxyMap.
- L30 `bool TryHandleInput(void* activePanel, int param_1, int param_2, int& rv)`: Owns Up/Down (cycle planet), Enter (travel), Esc (cancel), and the other nav keys (consumed as no-ops) on the galaxy map so the generic chain never walks the unnamed planet buttons.
- L34 `bool SpeakDescription(void* panel)`: Shift+Down peek: speak LBL_DESC for the currently displayed planet.
- L39 `void DispatchInput(void* panel, int engineCode, bool announcePlanet)`: Deferred engine dispatch, invoked from the pending-op Drain.
- L43 `void Tick()`: First-sight announce: when the galaxy map newly appears in panels[], speak the title + current planet name.
