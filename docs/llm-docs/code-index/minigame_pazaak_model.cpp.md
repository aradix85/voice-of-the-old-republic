# minigame_pazaak_model.cpp (163 lines)

Source: `patches/Accessibility/minigame_pazaak_model.cpp`

The pazaak game as the engine holds it: reading the panel, the model, the players and their cards, and calling the engine's own play, stand, end-turn and wager handlers.

## Declarations (in source order)

- L48 `bool ReadPtrAt(void* base, size_t off, void** out)`
- L58 `bool IsValueChoiceCard(int index)`: True for the one card whose FACE the player picks as well as its sign.
- L64 `int FirstMainDeckIndex()`: First main-deck index: 18 on KOTOR 1, 23 on KOTOR 2 (whose five special side-deck cards take 18..22).
- L66 `bool IsMainDeckCard(int index)`
- L69 `bool ReadIntAt(void* base, size_t off, int* out)`: ---- SEH-guarded primitive reads -----------------------------------------
- L76 `void WriteIntAt(void* base, size_t off, int v)`
- L82 `void* GetModel(void* panel)`
- L87 `int GetState(void* panel)`
- L92 `void* PlayerOf(void* model)`
- L96 `void* EnemyOf(void* model)`
- L100 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L109 `int CallGetTotal(void* player)`
- L116 `int BoardCount(void* player)`
- L123 `CardView BoardLast(void* player, int count)`
- L129 `void DoPlay(void* panel, int slot)`: ---- Engine action dispatch (thiscall, this = panel) ----------------------
- L134 `void DoStand(void* panel)`
- L139 `void DoEndTurn(void* panel)`
- L144 `bool IsSignChoiceCard(int index)`
- L149 `void DispatchWagerInput(void* panel, int code)`
