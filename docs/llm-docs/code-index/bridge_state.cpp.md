# bridge_state.cpp (211 lines)

Source: `patches/Accessibility/bridge_state.cpp`

Local bridge: the /state snapshot (area, position, party, screen, dialogue replies, target) and /menu.

## Declarations (in source order)

- L9 `std::string CreatureJson(void* serverCreature)`
- L30 `std::string BuildSnapshot()`
- L180 `std::string BuildMenu()`: The open screen as the mod's menu navigation sees it: every entry of the focus chain the arrow keys walk, with the focused one marked.
