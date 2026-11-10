# engine_reads.h (348 lines)

Source: `patches/Accessibility/engine_reads.h`

SEH-guarded read helpers for KOTOR GUI controls. Safe from hook handlers that may run during engine mid-teardown — every deref is __try-wrapped, every potentially-stale pointer is validated before use.

## Declarations (in source order)

- L16 `bool ReadControlNameFields(void* control, const char*& outTip, uint32_t& outTipLen, int& outId)`: True iff the control had non-empty tooltip text.
- L27 `void* CallDowncast(void* control, int vtableIndex)`: vtable[index](control) as __thiscall.
- L30 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`: CExoString at base+offset.
- L32 `uint32_t ReadU32(void* base, size_t offset)`
- L38 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`: SEH-guarded single-field reads for possibly-dead engine pointers (a non-null control pointer is NOT proof of life on KOTOR 2).
- L39 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L40 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L44 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: SEH-wrapped TLK lookup.
- L58 `bool EqualsTrimmed(const char* a, const char* b)`: Case-sensitive compare that ignores leading/trailing whitespace on both sides.
- L65 `bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize)`: Mirrors CSWGuiControl::DisplayToolTip @0x418a90: 1.
- L81 `bool ReadGuiString(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`: Read directly from CAurGUIStringInternal — the engine's actually- rendered string.
- L85 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`: Inline CExoString, then TLK strref fallback.
- L97 `bool ExtractTextOrStrRefIndirect(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`: Try every text path: 1.
- L107 `bool ReadLabelText(void* label, char* outBuf, size_t bufSize)`: Read a CSWGuiLabel's rendered text — engine's resolved gui_string path with the strref / CExoString / text_object fallback chain (i.e.
- L111 `inline bool ReadLabelTextAt(void* panel, size_t offset, char* outBuf, size_t bufSize)`: Convenience: panel + label-field offset, for the common case where a label is embedded in the panel struct at a known field.
- L124 `bool ReadButtonText(void* button, char* outBuf, size_t bufSize)`: Read a CSWGuiButton's rendered text — same two-step shape against the button's own field offsets (kButton{GuiStringPtr,Text,StrRef,TextObject} Offset).
- L128 `bool IsToggle(void* control)`: Used by ExtractAnnounceableText (which field reads) and IsChainNavigable (where chain nav can land).
- L129 `bool IsSlider(void* control)`
- L130 `bool IsListBox(void* control)`
- L131 `bool IsEditbox(void* control)`
- L135 `uint32_t ReadControlBitFlags(void* control)`: CSWGuiControl.bit_flags read, SEH-guarded.
- L140 `bool IsControlVisible(void* control)`: bit_flags & kControlVisibleBit — the engine's own "shown" bit, which panels with several stacked listboxes (store, crafting, messages) flip to pick the one on screen.
- L144 `int ReadListBoxRowCount(void* listBox)`: Row count of a CSWGuiListBox (its controls array size), -1 on fault or an implausible size.
- L148 `bool ReadToggleState(void* toggle)`: CSWGuiButtonToggle.field2_0x1c8 bit 0 — HandleInputEvent XORs on activate, Draw branches on (& 1) for which border to render.
- L152 `void DumpControlVtable(void* control, char* out, size_t outSize)`: vtable[0..7] dump.
- L158 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`: Convert a client-side object id to its server-side id via CServerExoApp::ClientToServerObjectId.
- L164 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`: Client-side handles (the kind in row+0x1c4 on CSWGuiStoreItemEntry, CSWGuiInGameItemEntry in Container/Equip-picker) → CSWSItem*.
- L169 `bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize)`: Multi-line property description (damage / range / on-hit / defence / base description).
- L176 `struct ItemDescriptionBlocks`: The four screen-reader navigation blocks of an item's property description, reconstructed by calling the engine's own per-category builders (the same ones GetPropertyDescription chains) into separate accumulators.
- L186 `bool BuildItemDescriptionBlocks(void* item, ItemDescriptionBlocks* out)`: Build the four blocks for `item` (a CSWSItem*).
- L195 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`: Workbench upgrade slot → installed-mod item.
- L207 `struct WorkbenchPickerInfo`: Layout of the workbench mod-picker (LB_ITEMS) for the currently-open slot.
- L213 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
- L230 `bool ResolveActionDescriptionFromActionId(uint32_t actionId, char* outBuf, size_t bufSize)`: CSWGuiInterfaceAction.action_id → human-readable description.
- L240 `int ReadItemRowStackCount(void* rowControl)`: Returns: > 1 stackable; caller speaks the count suffix.
- L249 `int ReadItemCharges(void* item)`: Charge readers — for limited-use consumables that carry a current / max charge count (CSWSItem.charges / max_charges).
- L250 `int ReadItemRowCharges(void* rowControl)`
- L263 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`: Name an item row from the ITEM it points at, not from its own caption.
- L275 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: Text of the CExoLocString at base+offset from a substring that is not English, for strings the English game cannot resolve.
- L281 `int ReadItemStack(void* item)`: Raw stack_size for a resolved CSWSItem* (0 on fault / infinite stock).
- L288 `void* ItemFromActionId(uint32_t actionId)`: Resolve the CSWSItem* behind an item-tagged CSWGuiInterfaceAction action_id (high nibble 0x4xxxxxxx, low 28 bits = server game-object id).
- L292 `void* GetFeatRow(uint16_t featIdx)`: The rules rows of a feat (CSWRules::GetFeat) and of a Force power (CSWSpellArray::GetSpell), or nullptr.
- L293 `void* GetSpellRow(int spellId)`
- L297 `bool ReadRowStrRefText(void* row, size_t offset, char* outBuf, size_t bufSize)`: The TLK text of the strref at `offset` of such a row; false when there is none.
- L302 `bool IsInventoryItemRow(void* control)`: CSWGuiInGameItemEntry rows (Inventory + Container loot listbox).
- L316 `bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax)`: Read a client creature's live Force pool.
- L326 `int ReadObjectDamageLevel(void* serverObject)`: The 0..5 visible wound-state bucket for a server object (0 healthy, 1 light, 2 wounded, 3 badly wounded, 4 dying, 5 dead), -1 when unreadable.
- L330 `int ReadObjectCurrentHitPoints(void* serverObject)`: Current hit points of a server object (CSWSObject +0xe0, signed short), -1 on null / fault.
- L335 `int ReadCreatureMaxHitPoints(void* serverCreature)`: Maximum hit points of a server creature (GetMaxHitPoints(1), the same call ReadObjectDamageLevel's KOTOR 2 path makes), -1 on null / fault.
- L340 `int ReadCreatureCurrentHitPoints(void* serverCreature)`: Current hit points through the engine accessor GetCurrentHitPoints(0) (current plus temporary), -1 on null / fault.
- L345 `bool IsPlotObject(void* serverObject)`: True when a server object carries the plot flag (indestructible).
