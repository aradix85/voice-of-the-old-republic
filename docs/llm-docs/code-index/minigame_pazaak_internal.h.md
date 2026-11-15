# minigame_pazaak_internal.h (161 lines)

Source: `patches/Accessibility/minigame_pazaak_internal.h`

Internal to the minigame_pazaak module (minigame_pazaak*.cpp); no other file includes it. The public interface is minigame_pazaak.h. Here: what the files share.

## Declarations (in source order)

- L90 `struct CardView`: `value` is CPazaakCard's third dword — KOTOR 2 only (its card is 12 bytes, KOTOR 1's is 8).
- L93 `struct Snap`: ---- Snapshot + delta announcements --------------------------------------
- L128 `bool IsValueChoiceCard(int index)`: minigame_pazaak_model.cpp
- L129 `int FirstMainDeckIndex()`
- L130 `bool IsMainDeckCard(int index)`
- L132 `void* GetModel(void* panel)`
- L133 `int GetState(void* panel)`
- L134 `void* PlayerOf(void* model)`
- L135 `void* EnemyOf(void* model)`
- L136 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L137 `int CallGetTotal(void* player)`
- L138 `int BoardCount(void* player)`
- L139 `CardView BoardLast(void* player, int count)`
- L140 `void DoPlay(void* panel, int slot)`
- L141 `void DoStand(void* panel)`
- L142 `void DoEndTurn(void* panel)`
- L145 `void FormatCard(const CardView& c, bool inHand, char* out, size_t n)`: minigame_pazaak_speech.cpp
- L146 `void Say(const char* s, bool interrupt = false)`
- L147 `void SpeakHand(void* player)`
- L148 `void SpeakTable(void* model)`
- L149 `void SpeakSignOption(int index, bool interrupt)`
- L150 `void SpeakOppHandCount(void* model)`
- L151 `void SpeakBoardSide(void* model, bool playerSide, bool interrupt = true)`
- L154 `void ConsumeConflicts()`: minigame_pazaak_input.cpp
- L155 `void PollShortcuts(void* panel)`
- L158 `void ServiceWagerPopup(void* fg)`: minigame_pazaak_wager.cpp

Also 39 constants or variables at namespace scope.
