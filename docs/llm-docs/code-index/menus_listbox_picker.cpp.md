# menus_listbox_picker.cpp (524 lines)

Source: `patches/Accessibility/menus_listbox_picker.cpp`

Armed-picker state and per-tick monitors for the two "select a row, then commit it into a slot" panels: the equipment picker (equip.gui LB_ITEMS) and the workbench upgrade picker (upgrade.gui LB_ITEMS).

Why these two are a pair, and why they are not in menus_listbox.cpp ------------------------------------------------------------------- Thirteen panels are driven by the spec table in menus_listbox.cpp. Eleven of them are stateless: the spec matches a panel, arrows drive its listbox, Enter commits, done. These two are the exception — they carry a mode. A picker ARMS when the user activates a slot button, and while armed it steals the arrow keys away from the panel's own button chain; when it disarms, arrows go back to the chain. That armed flag, the panel pointer it is bound to, and the one-shot cursor-park latch are the only mutable state the whole listbox subsystem owns.

State wants an owner, so it lives here with the two monitors that watch it per tick. The specs themselves stay in menus_listbox.cpp next to the other eleven — splitting two of thirteen specs out would have forced the private ListBoxPanelSpec struct into a shared header and left the dispatcher's probe table pointing at entries in another file. The spec callbacks reach this state through the accessors in menus_listbox.h instead, which is what the chain input files do from the outside.

The armed flag is the ENGINE's, not ours ---------------------------------------- Both panels already carry a "picker open" bit that the engine raises when it opens the item zone and clears when it closes it — kEquipPickerOpenFlagOff and kUpgradePickerOpenFlagOff. Crucially, on the equip screen the same call that raises the bit (ShowDescription(1)) is what clears the interactive bit on all nine slot buttons and labels, and the same call that clears it (CloseDescription → ShowDescription(0)) is what puts them back. So the bit is not merely a good proxy for "the picker is up" — it is the exact predicate that decides whether the slots underneath are usable.

This file used to keep its own bool alongside it, raised at keypress time and cleared at keypress time, while the engine's moved a tick later when the queued op drained. Every gap between the two produced a wrong answer, and one of them was permanent: Escape cleared OUR flag and never told the engine, so the engine's bit stayed set, the slot buttons stayed disabled, and every slot announced "unavailable" for the rest of the panel's life (K2 patch-20260817-065149.log, K1 patch-20260813-204335.log). The commit path had the same bug in miniature — one tick of "unavailable" between our disarm and OnOKPressed draining.

So: read the bit, never copy it. The only local state left is an ARM LATCH per picker, covering the one tick between queueing the engine's open call and the engine raising its own bit; it is self-limiting (the monitor drops it as soon as the bit appears, or as soon as the queued op has drained without one, which is also what happens when the slot had no items and the engine popped a message box instead). Plus the one-shot cursor park, which is an action to perform rather than a state to agree about.

The cursor-park mechanism ------------------------- Both pickers set useEngineSelect, i.e. arrows drive the engine's own CSWGuiListBox::SetSelectedControl rather than a raw selection_index write, so the user gets a real row highlight and native multipage scrolling. The catch is that the engine re-selects whatever row sits under the mouse on every frame (HandleMouseMove -> SetSelectedControl), which silently reverts our writes. Parking the OS cursor on the panel's BTN_BACK — clear of the list — makes that hover-select inert. The park is deferred to the monitor rather than done at arm time because MoveMouseToPosition recurses back through the hover pipeline and must stay off the input-dispatch stack.

## Declarations (in source order)

- L104 `bool ReadPickerOpenBit(void* panel, size_t fieldOff)`: Bit 0 of a panel's picker-open field.
- L120 `bool IsEquipPickerArmed()`: Both IsArmed queries are PURE — they are called from the input dispatcher, the extractor's disabled-suffix path and the msg-router rule, several times per frame, and a query that mutated the latch would make the answe...
- L127 `void ArmEquipPicker(void* panel, void* slotBtn)`
- L137 `void ClearEquipPickerArmLatch()`: "We are done driving this picker." Deliberately does NOT clear the engine's bit — the engine clears it itself when the commit or cancel we queued reaches CloseDescription, and the slots are genuinely still disabled un...
- L143 `bool IsWorkbenchUpgradePickerArmed()`
- L150 `void* WorkbenchUpgradePickerPanel()`
- L156 `void ArmWorkbenchUpgradePicker(void* panel)`
- L161 `void ClearWorkbenchUpgradeArmLatch()`
- L190 `bool ParkPickerCursorOffList(void* panel, void* backBtn, const char* tag)`: Warp the OS cursor onto the panel's BTN_BACK so it sits OFF the LB_ITEMS list while a picker is armed — see the cursor-park note in the file header.
- L207 `void TraceEquipPickerOpenField(void* panel, bool armed)`: Raw read of the equip panel's picker-open field for the diagnostic trace.
- L232 `void SpeakStillEquippedAfterRefusal(void* panel, void* slotBtn)`: The engine refused to open the picker — its "no items for this slot" modal is what the user just heard (the ContentChange monitor speaks it).
- L256 `void MonitorEquipPickerSelection()`: Equip picker: retire the arm latch, run the one-shot cursor park, and speak row changes.
- L402 `void MonitorWorkbenchUpgradePicker()`: Workbench upgrade picker: retire the arm latch and run the one-shot cursor park.
- L504 `bool RuleSuppressEquipPreviewFeedback(const char* text)`: Message-buffer rule: mute the engine's feedback while the equip picker is up.
- L513 `void RegisterPickerMsgRules()`
- L518 `void TickPickerMonitors()`
