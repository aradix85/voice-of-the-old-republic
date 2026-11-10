# minigame_pazaak_input.cpp (293 lines)

Source: `patches/Accessibility/minigame_pazaak_input.cpp`

The pazaak keys: moving between the hand and the boards, playing a card, standing, ending the turn, and the shortcut keys.

## Declarations (in source order)

- L18 `void PlayAndAnnounce(void* panel, void* player, int slot)`: Play the focused hand card and speak the confirmation.
- L34 `void ActStand(void* panel, int state)`: The board's two turn actions, with their one turn gate.
- L43 `void ActEndTurn(void* panel, int state)`
- L54 `int ZoneLen(void* model, int zone)`: ---- Arrow-zone navigation helpers ---------------------------------------
- L64 `bool HandSlotFilled(void* player, int slot)`: Hand slots can be empty (index < 0) when a card has been played or the hand holds fewer than four cards.
- L69 `int FirstHandCol(void* player)`
- L76 `int NextHandCol(void* player, int from, int dir)`: Next filled slot in `dir` (+1 right / -1 left); returns `from` if none (no wrap).
- L83 `void AnnounceZoneEntry(void* model, int zone)`: Up/Down entered a zone — read its summary so the user gets oriented.
- L92 `void AnnounceZoneElement(void* model, int zone, int col)`: Left/Right moved within a zone — read the single focused element.
- L113 `void HandleEnter(void* panel, void* model, int state)`: Enter on the focused zone element (driven from TryHandleInput).
- L150 `void ConsumeConflicts()`: Suppress the in-world / menu actions that share our physical keys so their pollers (which run later in Dispatch) don't act on the same press.
- L167 `void PollShortcuts(void* panel)`: Win32-polled shortcuts — letter scancodes the engine drops before the manager hook, so we read them directly (mirrors the in-world cycle keys).
- L192 `bool TryHandleInput(void* /*activePanel*/, int param_1, int param_2, int& rv)`: Arrow + Enter navigator, driven from the manager input hook so the generic chain can't also act on the board's controls.
