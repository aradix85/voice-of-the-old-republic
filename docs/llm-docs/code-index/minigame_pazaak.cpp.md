# minigame_pazaak.cpp (1122 lines)

Source: `patches/Accessibility/minigame_pazaak.cpp`

Pazaak minigame accessibility — see pazaak.h + docs/pazaak-investigation.md.

Design (no engine detour hook): * Identify the live board panel structurally — the foreground modal whose deep fields match the CSWPazaak model layout — then track it by pointer until it leaves the manager. The panel's vtable is learned on first sight and logged so it can be promoted to a hard constant later. * Read the model each tick and announce deltas. Pure state observation, so we poll rather than hook (hook-vs-poll principle). * Drive play through the engine's own CSWGuiPazaakGame handlers (HandlePlayHandCard / HandleStand / HandleContinue), which guard on game_state internally — safe to call off our OnUpdate tick (the same deferred callback site the menu pending-queue drains from). * Input is an arrow-zone navigator (same model as the deck builder): zone 0 your hand, 1 your board, 2 opponent board, 3 actions. Up/Down switch zones, Left/Right move within, Enter plays/activates. Arrows + Enter are handled through the manager input hook (TryHandleInput) and consumed, so the generic chain can't also drive the board's buttons. The letter shortcuts (s/e/c/t, Shift+C) are Win32-polled because the engine drops those scancodes before the manager hook.

## Declarations (in source order)

- L177 `bool IsValueChoiceCard(int index)`: True for the one card whose FACE the player picks as well as its sign.
- L183 `int FirstMainDeckIndex()`: First main-deck index: 18 on KOTOR 1, 23 on KOTOR 2 (whose five special side-deck cards take 18..22).
- L184 `bool IsMainDeckCard(int index)`
- L193 `bool ReadIntAt(void* base, size_t off, int* out)`: ---- SEH-guarded primitive reads -----------------------------------------
- L199 `bool ReadPtrAt(void* base, size_t off, void** out)`
- L205 `void WriteIntAt(void* base, size_t off, int v)`
- L211 `void* GetModel(void* panel)`
- L215 `int GetState(void* panel)`
- L219 `void* PlayerOf(void* model)`
- L222 `void* EnemyOf(void* model)`
- L231 `CardView ReadCard(void* player, size_t baseOff, int slot)`
- L240 `int CallGetTotal(void* player)`
- L247 `int BoardCount(void* player)`
- L253 `CardView BoardLast(void* player, int count)`
- L277 `bool LooksLikePazaak(void* panel)`: Structural identity probe — true iff `panel` looks like a CSWGuiPazaakGame.
- L306 `void FormatCard(const CardView& c, bool inHand, char* out, size_t n)`: ---- Card label synthesis (docs/pazaak-investigation.md §5) --------------- Thin wrapper over the shared synthesizer (acc::pazaak::FormatCardLabel, defined below the anonymous namespace and declared in pazaak.h).
- L312 `void Say(const char* s, bool interrupt = false)`
- L317 `void BuildBoard(void* player, char* out, size_t n)`: ---- On-demand readouts (c / t) ------------------------------------------
- L331 `void SpeakHand(void* player)`
- L350 `void SpeakTable(void* model)`
- L398 `int SignCardMagnitude(int index)`: Magnitude of a ±-faced card, for the sign chooser's spoken preview.
- L407 `void SpeakSignOption(int index, bool interrupt)`: Speak the sign chooser's current face ("plus 3" / "minus 1" / "plus 2").
- L416 `Snap ReadSnap(void* panel)`
- L435 `void AnnounceDeltas(void* panel, const Snap& cur)`
- L515 `void DoPlay(void* panel, int slot)`: ---- Engine action dispatch (thiscall, this = panel) ----------------------
- L519 `void DoStand(void* panel)`
- L523 `void DoEndTurn(void* panel)`
- L532 `void PlayAndAnnounce(void* panel, void* player, int slot)`: Play the focused hand card and speak the confirmation.
- L547 `void SpeakOppHandCount(void* model)`: The opponent's remaining hand cards, count only — the same public information a sighted player reads off the face-down cards, never the values.
- L563 `void ActStand(void* panel, int state)`: The board's two turn actions, with their one turn gate.
- L572 `void ActEndTurn(void* panel, int state)`
- L583 `int ZoneLen(void* model, int zone)`: ---- Arrow-zone navigation helpers ---------------------------------------
- L593 `bool HandSlotFilled(void* player, int slot)`: Hand slots can be empty (index < 0) when a card has been played or the hand holds fewer than four cards.
- L597 `int FirstHandCol(void* player)`
- L603 `int NextHandCol(void* player, int from, int dir)`: Next filled slot in `dir` (+1 right / -1 left); returns `from` if none (no wrap).
- L610 `void SpeakBoardSide(void* model, bool playerSide)`: Announce one board side as a whole ("Your board: ..., total N").
- L622 `void AnnounceZoneEntry(void* model, int zone)`: Up/Down entered a zone — read its summary so the user gets oriented.
- L631 `void AnnounceZoneElement(void* model, int zone, int col)`: Left/Right moved within a zone — read the single focused element.
- L655 `void ConsumeConflicts()`: Suppress the in-world / menu actions that share our physical keys so their pollers (which run later in Dispatch) don't act on the same press.
- L672 `void PollShortcuts(void* panel)`: Win32-polled shortcuts — letter scancodes the engine drops before the manager hook, so we read them directly (mirrors the in-world cycle keys).
- L694 `void HandleEnter(void* panel, void* model, int state)`: Enter on the focused zone element (driven from TryHandleInput).
- L748 `void StepWager(void* fg, int dir)`
- L754 `void ServiceWagerPopup(void* fg)`: Run every tick while the wager popup may be foreground (from Tick).
- L822 `void ResetState()`
- L838 `bool IsSignChoiceCard(int index)`
- L844 `void FormatFlipFaces(int mag, int flip, CardContext ctx, char* out, size_t n)`: Render one of the six classic ±N flip faces into `out`.
- L877 `void FormatCardLabel(int index, int flip, CardContext ctx, char* out, size_t n, int value)`: Shared card-label synthesizer — see pazaak.h.
- L937 `bool TryHandleInput(void* /*activePanel*/, int param_1, int param_2, int& rv)`: Arrow + Enter navigator, driven from the manager input hook so the generic chain can't also act on the board's controls.
- L1037 `bool IsBoardForeground()`
- L1039 `void DispatchWagerInput(void* panel, int code)`
- L1052 `void Tick()`
