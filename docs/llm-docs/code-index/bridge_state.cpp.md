# bridge_state.cpp (177 lines)

Source: `patches/Accessibility/bridge_state.cpp`

Local bridge: /state (area, position, party, screen, dialogue replies, target) and /menu.

## Declarations (in source order)

- L29 `void* ForegroundPanel()`
- L34 `std::string ScreenField(void* panel)`
- L40 `std::string CreatureJson(void* serverCreature)`
- L59 `std::string PartyJson()`: The controlled creature plus the other members, each once.
- L80 `std::string DialogRepliesField()`: The replies of a conversation waiting for a choice.
- L93 `std::string TargetField(const PlayerFrame& f)`
- L105 `std::string BuildSnapshot()`
- L152 `std::string BuildMenu()`: The open screen as the mod's menu navigation sees it: every entry of the focus chain the arrow keys walk, with the focused one marked.
