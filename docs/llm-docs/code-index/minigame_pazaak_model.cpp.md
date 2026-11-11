# minigame_pazaak_model.cpp (156 lines)

Source: `patches/Accessibility/minigame_pazaak_model.cpp`

The pazaak game as the engine holds it: reading the panel, the model, the players and their cards, and calling the engine's own play, stand, end-turn and wager handlers.

## Declarations (in source order)

- L48 `bool ReadPtrAt(void* base, size_t off, void** out)`
- L58 `bool IsValueChoiceCard(int index)`: True for the one card whose FACE the player picks as well as its sign.
- L64 `int FirstMainDeckIndex()`: First main-deck index: 18 on KOTOR 1, 23 on KOTOR 2 (whose five special side-deck cards take 18..22).
- L66 `bool IsMainDeckCard(int index)`
- L69 `void WriteIntAt(void* base, size_t off, int v)`: ---- SEH-guarded primitive reads -----------------------------------------
- L75 `void* GetModel(void* panel)`
- L80 `int GetState(void* panel)`
- L85 `void* PlayerOf(void* model)`
- L89 `void* EnemyOf(void* model)`
- L93 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L102 `int CallGetTotal(void* player)`
- L109 `int BoardCount(void* player)`
- L116 `CardView BoardLast(void* player, int count)`
- L122 `void DoPlay(void* panel, int slot)`: ---- Engine action dispatch (thiscall, this = panel) ----------------------
- L127 `void DoStand(void* panel)`
- L132 `void DoEndTurn(void* panel)`
- L137 `bool IsSignChoiceCard(int index)`
- L142 `void DispatchWagerInput(void* panel, int code)`
