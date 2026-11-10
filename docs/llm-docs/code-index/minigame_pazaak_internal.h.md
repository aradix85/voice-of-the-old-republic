# minigame_pazaak_internal.h (162 lines)

Source: `patches/Accessibility/minigame_pazaak_internal.h`

Internal to the minigame_pazaak module (minigame_pazaak*.cpp); no other file includes it. The public interface is minigame_pazaak.h. Here: what the files share.

## Declarations (in source order)

- L90 `struct CardView`: `value` is CPazaakCard's third dword — KOTOR 2 only (its card is 12 bytes, KOTOR 1's is 8).
- L93 `struct Snap`: ---- Snapshot + delta announcements --------------------------------------
- L128 `bool IsValueChoiceCard(int index)`: minigame_pazaak_model.cpp
- L129 `int FirstMainDeckIndex()`
- L130 `bool IsMainDeckCard(int index)`
- L131 `bool ReadIntAt(void* base, size_t off, int* out)`
- L132 `void WriteIntAt(void* base, size_t off, int v)`
- L133 `void* GetModel(void* panel)`
- L134 `int GetState(void* panel)`
- L135 `void* PlayerOf(void* model)`
- L136 `void* EnemyOf(void* model)`
- L137 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L138 `int CallGetTotal(void* player)`
- L139 `int BoardCount(void* player)`
- L140 `CardView BoardLast(void* player, int count)`
- L141 `void DoPlay(void* panel, int slot)`
- L142 `void DoStand(void* panel)`
- L143 `void DoEndTurn(void* panel)`
- L146 `void FormatCard(const CardView& c, bool inHand, char* out, size_t n)`: minigame_pazaak_speech.cpp
- L147 `void Say(const char* s, bool interrupt = false)`
- L148 `void SpeakHand(void* player)`
- L149 `void SpeakTable(void* model)`
- L150 `void SpeakSignOption(int index, bool interrupt)`
- L151 `void SpeakOppHandCount(void* model)`
- L152 `void SpeakBoardSide(void* model, bool playerSide, bool interrupt = true)`
- L155 `void ConsumeConflicts()`: minigame_pazaak_input.cpp
- L156 `void PollShortcuts(void* panel)`
- L159 `void ServiceWagerPopup(void* fg)`: minigame_pazaak_wager.cpp

Also 39 constants or variables at namespace scope.
