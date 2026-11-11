# minigame_pazaak_model.cpp (143 lines)

Source: `patches/Accessibility/minigame_pazaak_model.cpp`

The pazaak game as the engine holds it: reading the panel, the model, the players and their cards, and calling the engine's own play, stand, end-turn and wager handlers.

## Declarations (in source order)

- L52 `bool IsValueChoiceCard(int index)`: True for the one card whose FACE the player picks as well as its sign.
- L58 `int FirstMainDeckIndex()`: First main-deck index: 18 on KOTOR 1, 23 on KOTOR 2 (whose five special side-deck cards take 18..22).
- L60 `bool IsMainDeckCard(int index)`
- L63 `void* GetModel(void* panel)`: ---- SEH-guarded primitive reads -----------------------------------------
- L67 `int GetState(void* panel)`
- L72 `void* PlayerOf(void* model)`
- L76 `void* EnemyOf(void* model)`
- L80 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L89 `int CallGetTotal(void* player)`
- L96 `int BoardCount(void* player)`
- L103 `CardView BoardLast(void* player, int count)`
- L109 `void DoPlay(void* panel, int slot)`: ---- Engine action dispatch (thiscall, this = panel) ----------------------
- L114 `void DoStand(void* panel)`
- L119 `void DoEndTurn(void* panel)`
- L124 `bool IsSignChoiceCard(int index)`
- L129 `void DispatchWagerInput(void* panel, int code)`
