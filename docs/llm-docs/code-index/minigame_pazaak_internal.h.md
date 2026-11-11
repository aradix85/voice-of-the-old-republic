# minigame_pazaak_internal.h (163 lines)

Source: `patches/Accessibility/minigame_pazaak_internal.h`

Internal to the minigame_pazaak module (minigame_pazaak*.cpp); no other file includes it. The public interface is minigame_pazaak.h. Here: what the files share.

## Declarations (in source order)

- L91 `struct CardView`: `value` is CPazaakCard's third dword — KOTOR 2 only (its card is 12 bytes, KOTOR 1's is 8).
- L94 `struct Snap`: ---- Snapshot + delta announcements --------------------------------------
- L129 `bool IsValueChoiceCard(int index)`: minigame_pazaak_model.cpp
- L130 `int FirstMainDeckIndex()`
- L131 `bool IsMainDeckCard(int index)`
- L133 `void WriteIntAt(void* base, size_t off, int v)`
- L134 `void* GetModel(void* panel)`
- L135 `int GetState(void* panel)`
- L136 `void* PlayerOf(void* model)`
- L137 `void* EnemyOf(void* model)`
- L138 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L139 `int CallGetTotal(void* player)`
- L140 `int BoardCount(void* player)`
- L141 `CardView BoardLast(void* player, int count)`
- L142 `void DoPlay(void* panel, int slot)`
- L143 `void DoStand(void* panel)`
- L144 `void DoEndTurn(void* panel)`
- L147 `void FormatCard(const CardView& c, bool inHand, char* out, size_t n)`: minigame_pazaak_speech.cpp
- L148 `void Say(const char* s, bool interrupt = false)`
- L149 `void SpeakHand(void* player)`
- L150 `void SpeakTable(void* model)`
- L151 `void SpeakSignOption(int index, bool interrupt)`
- L152 `void SpeakOppHandCount(void* model)`
- L153 `void SpeakBoardSide(void* model, bool playerSide, bool interrupt = true)`
- L156 `void ConsumeConflicts()`: minigame_pazaak_input.cpp
- L157 `void PollShortcuts(void* panel)`
- L160 `void ServiceWagerPopup(void* fg)`: minigame_pazaak_wager.cpp

Also 40 constants or variables at namespace scope.
