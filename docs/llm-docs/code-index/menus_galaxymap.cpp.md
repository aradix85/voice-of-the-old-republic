# menus_galaxymap.cpp (200 lines)

Source: `patches/Accessibility/menus_galaxymap.cpp`

(From menus_galaxymap.h.) Galaxy / star-map travel screen accessibility (CSWGuiInGameGalaxyMap, galaxymap.gui — opened from the Ebon Hawk navigation console).

The panel is a single-axis "cycle one planet, then Travel / Cancel" surface. Its planet hotspots are image-only CSWGuiButtons with empty captions (they render as "control N" through the generic chain), and the real selection state lives server-side on CSWPartyTable, NOT on the panel — so the generic chain can neither name the planets nor tell which are hidden.

Instead we drive the engine's own CSWGuiInGameGalaxyMap::HandleInputEvent, whose NextPlanet/PrevPlanet handlers already skip planets the party table reports as unavailable (not yet revealed) or unselectable (can't travel there now). Up/Down cycle the reachable planets and announce each name from LBL_PLANETNAME; Enter travels (accept), Esc cancels (back); Shift+Down reads LBL_DESC (handled via peek_description). First sight speaks a fixed title plus the current planet name.

## Declarations (in source order)

- L49 `bool ReadLabel(void* panel, size_t labelOffset, char* outBuf, size_t bufSize)`: Read a CSWGuiLabel's rendered text (gui_string) at the given panel-relative offset into outBuf.
- L62 `void AnnouncePlanetName(void* panel, bool interrupt)`
- L75 `void* FindGalaxyMapInPanels()`
- L81 `bool IsGalaxyMapPanel(void* panel)`
- L85 `bool TryHandleInput(void* activePanel, int param_1, int param_2, int& rv)`
- L140 `bool SpeakDescription(void* panel)`
- L152 `void DispatchInput(void* panel, int engineCode, bool announcePlanet)`
- L176 `void Tick()`
