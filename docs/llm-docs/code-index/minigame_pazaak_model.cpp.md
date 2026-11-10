# minigame_pazaak_model.cpp (162 lines)

Source: `patches/Accessibility/minigame_pazaak_model.cpp`

The pazaak game as the engine holds it: reading the panel, the model, the players and their cards, and calling the engine's own play, stand, end-turn and wager handlers.

## Declarations (in source order)

- L47 `bool ReadPtrAt(void* base, size_t off, void** out)`
- L57 `bool IsValueChoiceCard(int index)`: True for the one card whose FACE the player picks as well as its sign.
- L63 `int FirstMainDeckIndex()`: First main-deck index: 18 on KOTOR 1, 23 on KOTOR 2 (whose five special side-deck cards take 18..22).
- L65 `bool IsMainDeckCard(int index)`
- L68 `bool ReadIntAt(void* base, size_t off, int* out)`: ---- SEH-guarded primitive reads -----------------------------------------
- L75 `void WriteIntAt(void* base, size_t off, int v)`
- L81 `void* GetModel(void* panel)`
- L86 `int GetState(void* panel)`
- L91 `void* PlayerOf(void* model)`
- L95 `void* EnemyOf(void* model)`
- L99 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L108 `int CallGetTotal(void* player)`
- L115 `int BoardCount(void* player)`
- L122 `CardView BoardLast(void* player, int count)`
- L128 `void DoPlay(void* panel, int slot)`: ---- Engine action dispatch (thiscall, this = panel) ----------------------
- L133 `void DoStand(void* panel)`
- L138 `void DoEndTurn(void* panel)`
- L143 `bool IsSignChoiceCard(int index)`
- L148 `void DispatchWagerInput(void* panel, int code)`
