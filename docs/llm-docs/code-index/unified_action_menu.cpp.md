# unified_action_menu.cpp (1550 lines)

Source: `patches/Accessibility/unified_action_menu.cpp`

kInputHome/End, kInputCatFirst/Last

## Declarations (in source order)

- L117 `int ClampInt(int v, int lo, int hi)`
- L133 `bool ForegroundPanelBlocks()`: True when a real engine panel / modal / dialog owns the foreground.
- L139 `int CountForCat(void* tam, void* mi, const Cat& c)`
- L145 `int& ShadowFor(const Cat& c)`
- L153 `void ApplySelection(void* tam, void* mi, const Cat& c, int idx)`: Stamp the engine's per-slot selection to `idx` so reads + dispatch land on the user's chosen entry (target: field1 via SelectActionInRow; personal: the +0x1bac selected-action-id via SelectVariant).
- L163 `void ReadLabel(void* tam, void* mi, const Cat& c, int idx, char* out, size_t n)`: Read the label of the selected entry.
- L173 `uint32_t ReadActionId(void* tam, void* mi, const Cat& c, int idx)`
- L184 `void AppendItemQuantity(void* tam, void* mi, const Cat& c, int idx, char* label, size_t n)`: Append ", N Stück" / ", N Ladungen" to an item entry's label, matching the inventory / store suffixes.
- L207 `bool Dispatch(void* tam, void* mi, const Cat& c)`
- L216 `const char* CategoryName(const Cat& c)`: Localised category name, or nullptr for an unnamed category (target rows on a non-creature target — door / placeable / trigger — whose rows carry per-object actions like Security / Bash and are announced by label).
- L249 `void BuildCategoryList(void* tam, void* mi)`
- L276 `int LocateCat(CatKind kind, int slot)`
- L283 `int FirstPopulatedTargetRow(void* tam)`
- L302 `bool TargetRowsLookHostileCreature(void* tam)`: Decide whether the target block's three rows are the hostile-creature Attacks / Force-Powers / Items layout (→ named categories) vs a door/placeable/trigger's per-object actions (→ announce by label).
- L315 `bool DetectCreature(void* tam, uint32_t handle)`: Combined creature signal: trust the clean vtable downcast for near targets, fall back to action-content for far / unresolvable ones.
- L324 `void FormatCategory(void* tam, void* mi, const Cat& c, int count, int idx, char* out, size_t n)`: Build the full category announce ("Name: label, N Optionen" / "label, N Optionen" for unnamed rows) into `out`.
- L352 `void SpeakCategory(void* tam, void* mi, const char* prefix)`: Speak the current category in full.
- L377 `void SpeakEntry(void* tam, void* mi)`: Speak only the current entry label — used after Up/Down/Home/End (the category didn't change, so its name would be redundant noise).
- L392 `uint32_t ResolveNarratedServerHandle()`: Server-side handle of the currently narrated target, or 0 if none / map pin / stale.
- L409 `bool ActionMenuAutoPauseEnabled()`: Vanilla parity: the radial / personal action menus auto-pause the world only when the "Action Menu" auto-pause option is on (CClientOptions bit 0x8000) — off by default.
- L414 `void Arm()`
- L437 `int PersonalSelection(int col)`
- L442 `int TargetSelection(int row)`
- L447 `bool IsActive()`
- L448 `bool IsSuspended()`
- L456 `void ReannounceCurrent()`: Re-speak the current category against the live menus.
- L485 `void SetForegroundBlocked(bool blocked)`: Panel-stack suspend / resume.
- L530 `void ForceDisarm(const char* reason)`
- L551 `bool ArmFromRadial(const char* name, uint32_t targetHandle)`
- L591 `bool OpenTarget(int row)`
- L664 `bool OpenPersonalImpl(int col, bool requested)`: `requested` distinguishes the two intents that share this body.
- L773 `bool OpenPersonal(int col)`
- L775 `bool OpenAnyPersonal()`
- L791 `void CloseFromEsc()`: Esc — close.
- L830 `bool ReanchorToNewTarget(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`: FOLLOW-CYCLING re-anchor: when the narrated target changed while the menu sat open (`,`/`.`/Q/E/passive all stamp the same slot), rebuild the target rows against the NEW target so the menu always shows that target's r...
- L875 `bool UnfoldTargetBlock(void* tam, uint32_t narrated, bool rowCarried[kRowCount])`: UNFOLD: the menu is personal-only (opened without a target, or its target was lost) and the user cycled onto something.
- L918 `void MaybeRestoreDrainedRows(void*& tam, bool targetChanged)`: LAZY re-anchor: re-anchoring on EVERY keypress broke target-menu navigation (the engine's per-press re-derivation churned the rows so arrows produced nothing).
- L935 `void DescribeSelectedEntry(void* tam, void* mi, const Cat& cur, int sel)`: Shift + any arrow — describe the selected entry, no movement.
- L963 `void RestampTargetRowForDispatch(void* tam, const Cat& cur)`: DoTargetAction dispatches at the creature_id baked into the matched list descriptor at PopulateMenus time — and while the menu sits open the engine re-bakes the lists against its own current target (combat reassert / ...
- L993 `uint32_t PendingRefusalStrRef(void* mi, const Cat& cur, int sel, bool* outRefused)`: Force the engine's APPEND path.
- L1009 `void SpeakRefusal(uint32_t strref)`: interrupt=false throughout: the bare-key path speaks the entry's label immediately before firing, and "Medikit" followed by the reason is the whole sentence the user needs — cutting the label off to deliver the reason...
- L1029 `bool DispatchWithQueueAppend(void* tam, void* mi, const Cat& cur)`
- L1073 `void ApplyPostFireClosePolicy()`: What happens to the menu after a successful fire.
- L1116 `void SpeakMedPickerRow(bool withTitle)`
- L1133 `bool FireMedicalDirect(void* mi, const Cat& c, int sel, void* targetCre, const char* how)`: Fire a Medicine-row entry at a specific CLIENT creature via the direct use-item request (DoPersonalAction is a dead end for K2 medical items — see SendUseItemRequestK2).
- L1158 `bool OpenMedPicker(void* mi, const Cat& c, int sel, const char* itemLabel)`: Returns true when the picker opened (the press is consumed).
- L1224 `bool HandleMedPickerInput(int code)`
- L1286 `bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel, bool targetChanged, bool selectionCarried)`: Follow-cycling contract: Enter right after cycling fires immediately when the user's selected action exists on the new target (selection carried by action_id above).
- L1366 `bool FirePersonal(int col)`: Fire a personal column's selected entry outright, with no menu.
- L1417 `bool HandleInputEvent(int code, int value)`
