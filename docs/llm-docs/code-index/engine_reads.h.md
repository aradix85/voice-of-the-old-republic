# engine_reads.h (386 lines)

Source: `patches/Accessibility/engine_reads.h`

SEH-guarded read helpers for KOTOR GUI controls. Safe from hook handlers that may run during engine mid-teardown — every deref is __try-wrapped, every potentially-stale pointer is validated before use.

## Declarations (in source order)

- L16 `bool ReadControlNameFields(void* control, const char*& outTip, uint32_t& outTipLen, int& outId)`: True iff the control had non-empty tooltip text.
- L21 `int ReadControlId(void* control)`: CSWGuiControl.id, the .gui "ID" property.
- L29 `uintptr_t ReadVtable(void* object)`: An object's vtable address: 0 for a null object and on a fault.
- L33 `bool HasVtable(void* object, uintptr_t vtable)`: The object is of the class with this vtable.
- L43 `void* CallDowncast(void* control, int vtableIndex)`: vtable[index](control) as __thiscall.
- L46 `bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize)`: CExoString at base+offset.
- L48 `uint32_t ReadU32(void* base, size_t offset)`
- L54 `bool TryReadU32(void* base, size_t offset, uint32_t* out)`: SEH-guarded single-field reads for possibly-dead engine pointers (a non-null control pointer is NOT proof of life on KOTOR 2).
- L55 `bool TryReadU16(void* base, size_t offset, uint16_t* out)`
- L56 `bool TryReadPtr(void* base, size_t offset, void** out)`
- L57 `bool TryReadFloat(void* base, size_t offset, float* out)`
- L61 `bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize)`: SEH-wrapped TLK lookup.
- L68 `bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize)`: Mirrors CSWGuiControl::DisplayToolTip @0x418a90: 1.
- L84 `bool ReadGuiString(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`: Read directly from CAurGUIStringInternal — the engine's actually- rendered string.
- L88 `bool ExtractTextOrStrRef(void* control, size_t cexoOffset, size_t strRefOffset, char* outBuf, size_t bufSize)`: Inline CExoString, then TLK strref fallback.
- L100 `bool ExtractTextOrStrRefIndirect(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`: Try every text path: 1.
- L110 `bool ReadLabelText(void* label, char* outBuf, size_t bufSize)`: Read a CSWGuiLabel's rendered text — engine's resolved gui_string path with the strref / CExoString / text_object fallback chain (i.e.
- L114 `inline bool ReadLabelTextAt(void* panel, size_t offset, char* outBuf, size_t bufSize)`: Convenience: panel + label-field offset, for the common case where a label is embedded in the panel struct at a known field.
- L127 `bool ReadButtonText(void* button, char* outBuf, size_t bufSize)`: Read a CSWGuiButton's rendered text — same two-step shape against the button's own field offsets (kButton{GuiStringPtr,Text,StrRef,TextObject} Offset).
- L131 `bool IsToggle(void* control)`: Used by ExtractAnnounceableText (which field reads) and IsChainNavigable (where chain nav can land).
- L132 `bool IsSlider(void* control)`
- L133 `bool IsListBox(void* control)`
- L134 `bool IsEditbox(void* control)`
- L138 `uint32_t ReadControlBitFlags(void* control)`: CSWGuiControl.bit_flags read, SEH-guarded.
- L143 `bool IsControlVisible(void* control)`: bit_flags & kControlVisibleBit — the engine's own "shown" bit, which panels with several stacked listboxes (store, crafting, messages) flip to pick the one on screen.
- L149 `int ReadListBoxRowCount(void* listBox)`: A CSWGuiListBox's rows.
- L151 `void* ReadListBoxRow(void* listBox, int index)`: Row `index`, nullptr out of range.
- L153 `int ReadListBoxSelection(void* listBox)`: selection_index; -1 is also the engine's "no selection".
- L155 `void* ReadListBoxSelectedRow(void* listBox)`: The selected row, nullptr without a selection.
- L157 `int FindListBoxRow(void* listBox, void* row)`: The index of `row` among the rows, -1 when it is not one.
- L163 `bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen)`: A CSWGuiEditbox's text.
- L167 `bool ReadToggleState(void* toggle)`: CSWGuiButtonToggle.field2_0x1c8 bit 0 — HandleInputEvent XORs on activate, Draw branches on (& 1) for which border to render.
- L171 `void DumpControlVtable(void* control, char* out, size_t outSize)`: vtable[0..7] dump.
- L177 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`: Convert a client-side object id to its server-side id via CServerExoApp::ClientToServerObjectId.
- L183 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`: Client-side handles (the kind in row+0x1c4 on CSWGuiStoreItemEntry, CSWGuiInGameItemEntry in Container/Equip-picker) → CSWSItem*.
- L188 `bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize)`: Multi-line property description (damage / range / on-hit / defence / base description).
- L195 `struct ItemDescriptionBlocks`: The four screen-reader navigation blocks of an item's property description, reconstructed by calling the engine's own per-category builders (the same ones GetPropertyDescription chains) into separate accumulators.
- L205 `bool BuildItemDescriptionBlocks(void* item, ItemDescriptionBlocks* out)`: Build the four blocks for `item` (a CSWSItem*).
- L214 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`: Workbench upgrade slot → installed-mod item.
- L226 `struct WorkbenchPickerInfo`: Layout of the workbench mod-picker (LB_ITEMS) for the currently-open slot.
- L232 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
- L249 `bool ResolveActionDescriptionFromActionId(uint32_t actionId, char* outBuf, size_t bufSize)`: CSWGuiInterfaceAction.action_id → human-readable description.
- L259 `int ReadItemRowStackCount(void* rowControl)`: Returns: > 1 stackable; caller speaks the count suffix.
- L268 `int ReadItemCharges(void* item)`: Charge readers — for limited-use consumables that carry a current / max charge count (CSWSItem.charges / max_charges).
- L269 `int ReadItemRowCharges(void* rowControl)`
- L282 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`: Name an item row from the ITEM it points at, not from its own caption.
- L294 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: Text of the CExoLocString at base+offset from a substring that is not English, for strings the English game cannot resolve.
- L300 `int ReadItemStack(void* item)`: Raw stack_size for a resolved CSWSItem* (0 on fault / infinite stock).
- L307 `void* ItemFromActionId(uint32_t actionId)`: Resolve the CSWSItem* behind an item-tagged CSWGuiInterfaceAction action_id (high nibble 0x4xxxxxxx, low 28 bits = server game-object id).
- L311 `void* GetFeatRow(uint16_t featIdx)`: The rules rows of a feat (CSWRules::GetFeat) and of a Force power (CSWSpellArray::GetSpell), or nullptr.
- L312 `void* GetSpellRow(int spellId)`
- L316 `bool ReadRowStrRefText(void* row, size_t offset, char* outBuf, size_t bufSize)`: The TLK text of the strref at `offset` of such a row; false when there is none.
- L321 `bool IsInventoryItemRow(void* control)`: CSWGuiInGameItemEntry rows (Inventory + Container loot listbox).
- L335 `bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax)`: Read a client creature's live Force pool.
- L343 `bool ReadClientCreatureHitPoints(void* clientCreature, int* outCur, int* outMax)`: A client creature's live hit points, from the stats block its character sheet renders (CSWCCreatureStats: +0x4c pregame_current_hp, +0x4e max_hit_points; +0x48 hit_points is the base value, not the live one).
- L349 `int ReadCreatureEffectIconIds(void* serverCreature, int* outIds, int cap)`: Icon ids of a server creature's effect-icon row (CSWSCreature.effect_icons: the icons its portrait shows, priority-sorted, one entry per icon id).
- L354 `int ReadObjectEffectTypes(void* serverObject, int* outTypes, int cap)`: EFFECT_TYPES of the first cap effects in CSWSObject.effects, in list order (a type can repeat).
- L364 `int ReadObjectDamageLevel(void* serverObject)`: The 0..5 visible wound-state bucket for a server object (0 healthy, 1 light, 2 wounded, 3 badly wounded, 4 dying, 5 dead), -1 when unreadable.
- L368 `int ReadObjectCurrentHitPoints(void* serverObject)`: Current hit points of a server object (CSWSObject +0xe0, signed short), -1 on null / fault.
- L373 `int ReadCreatureMaxHitPoints(void* serverCreature)`: Maximum hit points of a server creature (GetMaxHitPoints(1), the same call ReadObjectDamageLevel's KOTOR 2 path makes), -1 on null / fault.
- L378 `int ReadCreatureCurrentHitPoints(void* serverCreature)`: Current hit points through the engine accessor GetCurrentHitPoints(0) (current plus temporary), -1 on null / fault.
- L383 `bool IsPlotObject(void* serverObject)`: True when a server object carries the plot flag (indestructible).
