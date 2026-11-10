# engine_panels.h (387 lines)

Source: `patches/Accessibility/engine_panels.h`

In-game panel identity registry. Classifies CSWGuiPanel pointers against named CGuiInGame slots so menu code can branch on semantic kind instead of layout.

Chain: *(CAppManager**)0x7A39FC → CAppManager → +0x4 CClientExoApp → +0x4 Internal → +0x40 CGuiInGame*. Re-resolved on every call so module transitions don't strand stale pointers.

## Declarations (in source order)

- L17 `enum class PanelKind`: Mirrors CGuiInGame's named slots.
- L177 `const char* PanelKindName(PanelKind k)`
- L181 `void* ResolveGuiInGame()`: AppManager → ClientExoApp → Internal → CGuiInGame.
- L188 `void* ResolveMainInterface()`: CGuiInGame → CSWGuiMainInterface (+0x90).
- L196 `bool ReadDialogReplyText(int replyIndex, char* outBuf, size_t bufSize)`: Read the resolved text of the current dialog entry's reply at `replyIndex` from CGuiInGame's render-independent reply-text array (see kCGuiInGameReplyTextArrayOffset).
- L202 `int ReadDialogReplyCount()`: Number of active replies in CGuiInGame's render-independent reply-text array (field69_0x114).
- L206 `PanelKind IdentifyPanel(void* panel)`: PanelKind::Unknown on no match.
- L208 `bool IsPanelKindInGameMenu(void* panel)`
- L224 `void* FindPanelByKind(PanelKind kind)`: First panel in CSWGuiManager.panels[] whose IdentifyPanel is `kind`, or nullptr.
- L233 `bool IsModalPopupPanel(PanelKind k)`: Engine-pushed standalone modal popups whose dismissal requires our Esc-routes- to-close handler (the engine's own Esc handling on these is to open the quit-confirm sibling, leaving the user stacked deeper instead of b...
- L240 `bool IsMainMenuOptionsSubScreen(PanelKind k)`: True for the title-screen Options sub-screens (Sound, Graphics, their Advanced variants, Auto-Pause, Feedback, Game, Mouse, Keyboard).
- L244 `bool HasActiveDialogPanel()`: During reply turns the engine swaps fg to a Fade overlay while the real dialog panel stays in panels[].
- L251 `bool HasActiveBarkBubble()`: True when a bark bubble is currently in the panel stack.
- L255 `bool HasActiveSubScreen()`: Same reason as HasActiveDialogPanel — drilled sub-screens hide under stale Fade overlays.
- L259 `bool HasActiveMapPanel(void** outPanel = nullptr)`: InGameMap hides under the InGameMenu strip when drilled.
- L265 `bool HasActiveLevelUpPanel()`: True iff a CSWGuiLevelUpPanel (InGameLevelUp) sits anywhere in panels[].
- L288 `bool LookupUpgradeSlotType(int slotCustomValue, int category, int* outUpgradeType, uint32_t* outStrRef)`: Resolve a workbench slot to its type + display strref out of the engine's slot-type table (kAddrUpgradeSlotTypeTable).
- L299 `bool IsInGameOptionsSubScreen(void* panel)`: Heap-allocated Options sub-screen (Gameplay / Graphics / Sound / Auto-Pause / Feedback / Keymapping / Mouse settings).
- L305 `bool CallPrevSWInGameGui()`: CGuiInGame::PrevSWInGameGui @0x0062cdf0 — engine's "back to strip" primitive.
- L310 `bool SetGlobalDialogState(int state)`: CGuiInGame::SetGlobalDialogState — force the engine's "conversation active" bit.
- L318 `bool CloseInGameMenuToWorld()`: Close the in-game menu (strip + drilled sub-screen) back to the world, replicating a menu tab's own Escape exactly: HideSWInGameGui(0) followed by SetInputClass(0,1).
- L321 `int GetInputClass()`: Read the live client input_class (0/4 = in-world, 2 = menu/GUI owns input).
- L326 `bool SetGuiInputClass(int klass)`: Set the client input class via the engine setter.
- L337 `enum class UiBlockReason`: True iff foreground UI is capturing input.
- L344 `struct UiBlockState`
- L352 `bool IsForegroundUiBlocking(UiBlockState* outState = nullptr)`: outState (optional) carries diagnostic detail for the log line.
- L365 `bool IsWorldLevelForegroundKind(PanelKind k)`: True for the handful of foreground kinds that coexist with normal in-world play — the HUD, the pause overlay, a bark bubble, a fade, and the unclassified Unknown.
- L384 `bool HasVtable(void* obj, uintptr_t expected)`: Verify an object's vtable pointer matches `expected`.
