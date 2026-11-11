# engine_actionbar.cpp (391 lines)

Source: `patches/Accessibility/engine_actionbar.cpp`

(From engine_actionbar.h.) Engine bindings for the player action bar (action menu).

Pure read + primitive layer. Mirrors engine_radial: resolve through the standard chain, per-column reads, drive engine widgets via the vtable[15] activate path. actionbar_menu wires this into a navigable Shift+4..Shift+7 submenu.

CSWGuiMainInterface.field45_0x771c[6] — six CSWGuiMainInterfaceAction (stride 0x71C). Each column: +0x000 action_button   — icon (fire = use) +0x1C4 action_label    — text label of current variant +0x388 up_button       — cycle next +0x54C down_button     — cycle prev +0x718 is_action       — 1 when populated

CSWGuiMainInterface.field5_0x74[6] — CExoArrayList<CSWGuiInterfaceAction> stride 0x0C. Source of truth for variant count (PopulateMenus fills from CSWCCreature::GetPersonalActions).

CSWGuiMainInterface::DoPersonalAction @0x0068ad60 — what bare 4..7 hit. param_2 unused — variant chosen by reading *(mi + 0x1bac + slot*4).

## Declarations (in source order)

- L91 `void* DescriptorAddr(void* mi, int slot, int index)`: Address of the descriptor list entry at `index` within slot's personal-actions list.
- L108 `bool ReadCExoStringLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: CExoString (c_string + length) read with NUL-termination guarantee.
- L136 `void* ResolveMainInterface()`: Kept as this module's public name — nine callers, including input_pipeline.cpp.
- L140 `int VariantCount(void* mi, int slot)`
- L150 `bool ReadVariantLabel(void* mi, int slot, int index, char* outBuf, size_t bufSize)`
- L159 `bool VariantRefusal(void* mi, int slot, int index, uint32_t* outStrRef)`
- L210 `uint32_t ReadVariantActionId(void* mi, int slot, int index)`
- L218 `void LogDispatchDiag(void* mi, int slot, int index)`
- L260 `bool EntryIsMedicalK2(void* mi, int slot, int index)`
- L269 `bool SendUseItemRequestK2(uint32_t actionId, void* targetClientCreature)`
- L310 `bool SelectVariant(void* mi, int slot, int index)`
- L322 `bool FireSelectedVariant(void* mi, int slot)`
- L338 `void LogState(void* mi, const char* tag)`
- L355 `bool PrepareBareDispatch(uint32_t targetClientHandle)`
