# minigame_pazaak_input.cpp (303 lines)

Source: `patches/Accessibility/minigame_pazaak_input.cpp`

The pazaak keys: moving between the hand and the boards, playing a card, standing, ending the turn, and the shortcut keys.

## Declarations (in source order)

- L20 `void PlayAndAnnounce(void* panel, void* player, int slot)`: Play the focused hand card and speak the confirmation.
- L36 `void ActStand(void* panel, int state)`: The board's two turn actions, with their one turn gate.
- L45 `void ActEndTurn(void* panel, int state)`
- L56 `void CloseSignChooser()`: Closes the sign chooser of a plus-or-minus card.
- L61 `void SpeakAction(int col)`
- L66 `int ZoneLen(void* model, int zone)`
- L76 `bool HandSlotFilled(void* player, int slot)`: Hand slots can be empty (index < 0) when a card has been played or the hand holds fewer than four cards.
- L81 `int FirstHandCol(void* player)`
- L88 `int NextHandCol(void* player, int from, int dir)`: Next filled slot in `dir` (+1 right / -1 left); returns `from` if none (no wrap).
- L95 `void AnnounceZoneEntry(void* model, int zone)`: Up/Down entered a zone — read its summary so the user gets oriented.
- L103 `void AnnounceZoneElement(void* model, int zone, int col)`: Left/Right moved within a zone — read the single focused element.
- L124 `void HandleEnter(void* panel, void* model, int state)`: Enter on the focused zone element (driven from TryHandleInput).
- L161 `void ConsumeConflicts()`: Suppress the in-world / menu actions that share our physical keys so their pollers (which run later in Dispatch) don't act on the same press.
- L178 `void PollShortcuts(void* panel)`: Win32-polled shortcuts — letter scancodes the engine drops before the manager hook, so we read them directly (mirrors the in-world cycle keys).
- L203 `bool TryHandleInput(void* /*activePanel*/, int param_1, int param_2, int& rv)`: Arrow + Enter navigator, driven from the manager input hook so the generic chain can't also act on the board's controls.
