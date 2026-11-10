# engine_actionbar.h (144 lines)

Source: `patches/Accessibility/engine_actionbar.h`

Engine bindings for the player action bar (action menu).

Pure read + primitive layer. Mirrors engine_radial: resolve through the standard chain, per-column reads, drive engine widgets via the vtable[15] activate path. actionbar_menu wires this into a navigable Shift+4..Shift+7 submenu.

CSWGuiMainInterface.field45_0x771c[6] — six CSWGuiMainInterfaceAction (stride 0x71C). Each column: +0x000 action_button   — icon (fire = use) +0x1C4 action_label    — text label of current variant +0x388 up_button       — cycle next +0x54C down_button     — cycle prev +0x718 is_action       — 1 when populated

CSWGuiMainInterface.field5_0x74[6] — CExoArrayList<CSWGuiInterfaceAction> stride 0x0C. Source of truth for variant count (PopulateMenus fills from CSWCCreature::GetPersonalActions).

CSWGuiMainInterface::DoPersonalAction @0x0068ad60 — what bare 4..7 hit. param_2 unused — variant chosen by reading *(mi + 0x1bac + slot*4).

## Declarations (in source order)

- L33 `void* ResolveMainInterface()`: Borrowed pointer — re-resolve each tick, don't cache.
- L39 `int VariantCount(void* mainInterface, int slot)`: Reads field5_0x74[slot].size.
- L43 `bool ReadVariantLabel(void* mainInterface, int slot, int index, char* outBuf, size_t bufSize)`: CSWGuiInterfaceAction.label CExoString at +0x00, stride 0x38.
- L47 `uint32_t ReadVariantActionId(void* mainInterface, int slot, int index)`: CSWGuiInterfaceAction +0x08 (ulong).
- L67 `bool VariantRefusal(void* mainInterface, int slot, int index, uint32_t* outStrRef)`: Will DoPersonalAction refuse this entry, and why?
- L78 `bool SelectVariant(void* mainInterface, int slot, int index)`: Stamps *(mi + 0x1bac + slot*4) = descriptor[index].action_id, then DoPersonalAction reads that field and searches for matching id.
- L81 `bool FireSelectedVariant(void* mainInterface, int slot)`: Same entry as bare 4..7.
- L91 `void LogDispatchDiag(void* mainInterface, int slot, int index)`: One log line dumping everything the engine's DoPersonalAction will gate on for this entry — action_id, creature_id (+ whether the client object array resolves it), handler dword, flag word.
- L95 `bool EntryIsMedicalK2(void* mainInterface, int slot, int index)`: True iff this entry carries KOTOR 2's medical-use handler (the two-step target-pick flow below).
- L113 `bool SendUseItemRequestK2(uint32_t actionId, void* targetClientCreature)`: KOTOR 2 medical-item use, sent directly.
- L139 `bool PrepareBareDispatch(uint32_t targetClientHandle)`: Prep engine state so the engine's bare 1..3 / 4..7 dispatch fires against targetClientHandle instead of whatever target was last stamped.
- L141 `void LogState(void* mainInterface, const char* tag)`

Also 1 constants or variables at namespace scope.
