# bridge_state.cpp (188 lines)

Source: `patches/Accessibility/bridge_state.cpp`

Local bridge: /state (area, position, party, screen, dialogue replies, target) and /menu.

## Declarations (in source order)

- L29 `void* ForegroundPanel()`
- L34 `std::string ScreenField(void* panel)`
- L40 `bool IsDialogPanel(void* panel)`
- L50 `std::string CreatureJson(void* serverCreature)`
- L69 `std::string PartyJson()`: The controlled creature plus the other members, each once.
- L90 `std::string DialogRepliesField()`: The replies of a conversation waiting for a choice.
- L103 `std::string TargetField(const PlayerFrame& f)`
- L115 `std::string BuildSnapshot()`
- L163 `std::string BuildMenu()`: The open screen as the mod's menu navigation sees it: every entry of the focus chain the arrow keys walk, with the focused one marked.
