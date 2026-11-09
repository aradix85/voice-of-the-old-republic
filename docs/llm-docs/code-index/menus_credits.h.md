# menus_credits.h (56 lines)

Source: `patches/Accessibility/menus_credits.h`

virtual credits row for Inventory + Store.

Sighted players see "Credits: 1247" in the top-right of CSWGuiInGameInventory and CSWGuiStore (both .gui files have an LBL_CREDITS heading + LBL_CREDITS_ VALUE label written by the engine at panel-open time). Labels aren't IsChainNavigable, so the chain walker would skip them — we surface them as virtual text-only chain entries the same way menus_charsheet does for the stat block.

Read path: the inline CSWGuiLabel.gui_string holds the engine's actually- rendered text. For Store, the same value is also cached on the panel struct at +0x2270 (kStorePlayerGoldOffset, written by PopulateStore via CSWSCreature::GetGold) and consumed by ReadStorePlayerGold — but for the chain row we just read the rendered label, which works for both kinds with no per-panel branching.

Wired by: * menus_chain.cpp — RebindChain: ForEachCreditsRowAnchor registers a text-only chain entry on credits_value_label. * menus_extract.cpp — FromControl section 0: IsCreditsRowAnchor + ExtractCreditsRow override the bare label text with the localised "Credits: 1247" phrase.

## Declarations (in source order)

- L34 `bool IsCreditsRowAnchor(void* panel, void* labelControl)`: True iff `labelControl` is the credits_value_label inline-struct member of `panel`, for a panel kind where we surface the credits row (Inventory / Store).
- L42 `void ForEachCreditsRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`: Iterate the credits-row anchors registered for `panel`.
- L52 `bool ExtractCreditsRow(void* panel, void* labelControl, char* outBuf, size_t bufSize)`: Read the credits_value_label rendered text and format the localised line ("Credits: 1247").
