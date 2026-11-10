# bridge_state.cpp (209 lines)

Source: `patches/Accessibility/bridge_state.cpp`

Local bridge: /state (area, position, party, screen, dialogue replies, target) and /menu.

## Declarations (in source order)

- L29 `void* ForegroundPanel()`
- L34 `std::string ScreenField(void* panel)`
- L40 `bool IsDialogPanel(void* panel)`
- L50 `std::string CreatureJson(void* serverCreature)`
- L67 `void* FindPlayerCharacter(void* area)`: The player character found in the area by its chargen name.
- L89 `std::string PartyJson(void* area)`: The controlled creature plus the NPC members, each once.
- L111 `std::string DialogRepliesField()`: The replies of a conversation waiting for a choice.
- L124 `std::string TargetField(const PlayerFrame& f)`
- L136 `std::string BuildSnapshot()`
- L184 `std::string BuildMenu()`: The open screen as the mod's menu navigation sees it: every entry of the focus chain the arrow keys walk, with the focused one marked.
