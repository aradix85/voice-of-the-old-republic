# menus_listbox.cpp (2261 lines)

Source: `patches/Accessibility/menus_listbox.cpp`

listbox-driven panel input dispatcher.

See menus_listbox.h for the motivation. This file holds:

* The ListBoxPanelSpec struct (private) — one entry per panel kind. * All thirteen spec entries. Each carries ~5-6 small static callbacks that capture its quirks (announce format, button IDs, custom Enter dispatch, etc.). * The dispatcher TryHandleInput that walks the table. * The container monitor + the container give-mode key poll.

The two armed pickers (equipment, workbench upgrade) keep their SPECS here with the other eleven, but their mutable state and per-tick monitors live in menus_listbox_picker.cpp — see that file's header. The spec callbacks reach the state through the accessors in menus_listbox.h.

## Declarations (in source order)

- L222 `bool ContainerMatches(void* p)`
- L226 `void* ContainerFindLb(void* p)`
- L232 `void ContainerAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Inline announce only on a no-op clamp (boundary).
- L264 `bool ContainerOnEnter(void* panel)`
- L285 `bool ContainerOnEsc(void* panel)`
- L325 `bool SaveLoadMatches(void* p)`: K1 only, deliberately.
- L329 `void* SaveLoadFindLb(void* p)`
- L340 `void SaveLoadAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak on every step (no per-tick monitor watches the SaveLoad listbox).
- L365 `void SaveLoadLogExtra(char* out, size_t outN, const ListBoxNavResult& r)`
- L374 `bool SaveLoadOnEnter(void* panel)`
- L395 `bool SaveLoadOnEsc(void* panel)`
- L425 `bool EquipPickerMatchesPanel(void* p)`
- L429 `bool EquipPickerArmed()`
- L437 `void* EquipPickerFindLb(void* p)`
- L448 `void EquipPickerAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Inline announce only on no-op clamp; normal moves are caught by MonitorEquipPickerSelection (menus_listbox_picker.cpp).
- L461 `void EquipPickerLogExtra(char* out, size_t outN, const ListBoxNavResult& r)`
- L468 `bool EquipPickerOnEnter(void* panel)`: Custom Enter: lookup row + btn from listbox.selection_index, queue an EquipCommit op, disarm.
- L556 `bool EquipPickerOnEsc(void* panel)`: Custom Esc: run the ENGINE's cancel, then drop our latch.
- L608 `bool SkillInfoBoxMatches(void* p)`
- L612 `void* SkillInfoBoxFindLb(void* p)`
- L620 `void* FindFeatsCharGenPanel()`: Walk the manager's panels[] for a CSWGuiFeatsCharGen instance.
- L647 `int ResolveFeatIdFromRowStrref(void* row, int& outRowStrref)`: Reverse-lookup: given the strref the engine wrote onto a SkillEntry row (kLabelStrRefOffset within the SkillEntry — its label_hilight.label.text .text_params.str_ref), find the matching feat in Rules->feats[] and retu...
- L676 `void SkillInfoBoxAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Standard per-row speech: just "feat name, N of M".
- L694 `void SkillInfoBoxEnrichRow(void* /*panel*/, const ListBoxNavResult& r)`: Per-row enrichment: refresh the underlying CSWGuiFeatsCharGen's description_listbox via OnEnterFeat(featId), then read + speak that description.
- L770 `bool SkillInfoBoxOnEnter(void* panel)`
- L786 `bool SkillInfoBoxTitleIsPlaceholder(void* panel, char* outText, size_t outSize)`: Read the panel's own title label, and say whether it still holds the BioWare dev-leftover baked into skillinfo.gui ("Items Available to Place in Container and blah blah blah").
- L802 `const char* SkillInfoBoxTitleOverride(void* panel)`: Title override: substitute the localised "Du erhÃ¤ltst diese Talente" string for the BioWare placeholder text baked into skillinfo.gui.
- L881 `void* MsgViewListBox(void* panel, MsgView v)`
- L893 `void* MsgViewButton(void* panel, MsgView v)`: The engine view button that shows `v`.
- L906 `bool ReadMsgViewName(void* panel, MsgView v, char* out, size_t outN)`: The name the screen itself shows for the stop: the view button's caption, or for the two effects lists their headings ("Gute Effekte" / ...).
- L916 `MsgView CurrentMsgView(void* panel)`
- L934 `void FormatMsgView(void* panel, MsgView v, char* out, size_t outN)`: " , N Einträge" / " , leer".
- L947 `bool InGameMessagesOnLeftRight(void* panel, int dir)`
- L980 `bool InGameMessagesMatches(void* p)`
- L984 `void* InGameMessagesFindLb(void* p)`
- L996 `void SpeakRowAndPosition(const ListBoxNavResult& r)`: The plain " , N von M" announce.
- L1013 `void InGameMessagesAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: NB: no `rowCount <= 0` guard here, unlike the other two users of SpeakRowAndPosition.
- L1017 `const char* InGameMessagesTitleOverride(void* panel)`
- L1056 `bool DialogCinematicMatches(void* p)`
- L1059 `bool DialogCinematicCopyMatches(void* p)`
- L1062 `bool DialogComputerMatches(void* p)`
- L1065 `bool DialogComputerCameraMatches(void* p)`
- L1069 `void* DialogFindRepliesLb(void* p)`
- L1106 `bool DialogRepliesLive(void* /*panel*/)`: The engine populates the CGuiInGame reply store (SetReplyData) only at the reply break — when the entry's VO ends and the choices actually appear.
- L1114 `constexpr ListBoxPanelSpec MakeDialogSpec(const char* logTag, bool (*matches)(void* panel))`: All four dialog variants take the identical spec — same reply-listbox locator, no announce (see above), no Enter/Esc/title handling.
- L1154 `bool WorkbenchItemsMatches(void* p)`
- L1158 `void* WorkbenchItemsFindLb(void* p)`
- L1164 `void WorkbenchItemsAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak the focused weapon row + position.
- L1169 `bool WorkbenchItemsOnEnter(void* panel)`
- L1175 `bool WorkbenchItemsOnEsc(void* panel)`
- L1227 `bool WorkbenchUpgradeMatches(void* p)`
- L1231 `bool WorkbenchUpgradeArmed()`
- L1238 `void* WorkbenchUpgradeFindLb(void* p)`
- L1254 `bool IsDashOrBlankRowText(const char* s)`: LB_ITEMS rows are CSWGuiInventoryItemEntry-style — their text comes from the item resref's localised name.
- L1271 `bool IsK2UpgradeNoneRow(void* row)`: K2 only: is this picker row the engine's "-" none/remove entry?
- L1281 `void WorkbenchUpgradeAnnounce(void* /*lb*/, const ListBoxNavResult& r)`
- L1325 `int WorkbenchUpgradeMinSel(void* panel)`: Dynamic minSel: hide the row-0 remove entry on power slots (minSel 1); show every row on the colour slot (minSel 0).
- L1340 `bool WorkbenchUpgradeOnEnter(void* panel)`: Enter (armed): commit the currently-selected LB_ITEMS row via a single pending op that fires vtable[15] on the row (stage), then on BTN_ASSEMBLE (install).
- L1417 `bool WorkbenchUpgradeOnEsc(void* panel)`: Esc (armed): disarm only.
- L1436 `const char* WorkbenchUpgradeTitleOverride(void* panel)`: The generic title-walk would land on the first leftover placeholder label (id 19 "Item Property" — a design-time string the engine repopulates per slot at runtime, alongside id 20 "Slot Name" / id 21 "Lightsaber Slot ...
- L1486 `bool ExamineMatches(void* p)`
- L1490 `void* ExamineFindLb(void* p)`
- L1500 `void ExamineAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: The `rowCount <= 0` guard is this spec's own, and InGameMessagesAnnounce deliberately does not have it.
- L1555 `bool ScriptSelectMatches(void* p)`
- L1559 `void* ScriptSelectFindLb(void* p)`
- L1567 `void ScriptSelectAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak the behaviour name + position, dropping the toggle's ", ein"/", aus" state suffix.
- L1597 `void ScriptSelectEnrichRow(void* panel, const ListBoxNavResult& r)`: Per-row enrichment: speak the behaviour's description.
- L1630 `bool ScriptSelectOnEnter(void* panel)`: Enter → Wählen: applies the selected ai_state and closes.
- L1636 `bool ScriptSelectOnEsc(void* panel)`
- L1704 `bool DispatchKeyDownEdge(const ListBoxPanelSpec& spec, void* panel, int param_1)`
- L1798 `void LogStandard(int n, void* thisPtr, int param_1, int param_2, bool consumed)`
- L1817 `bool TryHandleInput(int n, void* thisPtr, void* activePanel, int param_1, int param_2, int& outRv)`
- L1880 `void MonitorContainerSelection()`
- L1975 `void PollContainerGiveModeKey()`
- L2005 `int FindListBoxRowIndex(void* listbox, void* control)`: Index of `control` among the listbox's row children, or -1.
- L2025 `bool SetListBoxSelectedControl(void* listbox, int index)`: The engine's own selection commit — real row highlight, native scroll, and the onSelectionChanged work the preview labels depend on.
- L2053 `void MonitorK2SaveLoadSelection()`: KOTOR 2 save/load: tell the engine which row the keyboard is on.
- L2112 `void AnnounceK2SaveLoadInfo()`: KOTOR 2 save/load: the per-slot location, one tick after the sync.
- L2165 `bool ReadK2SaveLoadSwapState(void* panel, void* lb, int* outIdx, int* outRows)`: One SEH block for every raw engine read this monitor needs.
- L2188 `void MonitorK2SaveLoadCharacterSwap()`
- L2240 `void TickListboxMonitors()`
- L2249 `const char* GetTitleOverride(void* panel)`
