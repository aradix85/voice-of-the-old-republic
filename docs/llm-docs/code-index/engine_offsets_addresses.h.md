# engine_offsets_addresses.h (1046 lines)

Source: `patches/Accessibility/engine_offsets_addresses.h`

Engine executable addresses: .text functions, vtables, and .data globals.

Part of the engine_offsets.h family - see engine_offsets_types.h for the split rationale and the full file list.

Functions and vtables (.text / .rdata) come first; the global pointers (.data) have their own section at the bottom of this file.

Every constant here is `const uintptr_t`, not `constexpr` - the marker is a runtime call resolved at load time against the detected game.

KOTOR 2. Each address is in one of two states, and there is no third: a recompile relocates everything, so a KOTOR 1 address is NEVER also valid in KOTOR 2 (unlike a struct offset, which often is).

.text / .rdata:  R(k1)                 - KOTOR 2 value not yet found -> 0 Pick(k1, k2)          - known .data:           TodoGlobal(k1)        - not yet found -> 0 PickGlobal(k1, k2)    - known

A .data pointer must never be left bare `constexpr`: that hands out the KOTOR 1 address on KOTOR 2, which is a plausible-looking pointer into unrelated data rather than an obvious failure.

## Declarations (in source order)

- L366 `typedef void (__thiscall* PFN_CSWGuiListBoxSetSelectedControl)(void* listbox, int index, int playSound)`: CSWGuiListBox::SetSelectedControl @0x0041c040 — __thiscall(listbox, index, playSound).
- L388 `typedef CExoString* (__thiscall* PFN_GetSimpleString)(void* this_, CExoString* out, uint32_t strref)`: CTlkTable::GetSimpleString — resolves a TLK str_ref to a localized string.
- L437 `typedef void (__thiscall* PFN_InGameEquipOnEnterSlot)(void* panel, void* slot_btn)`: CSWGuiInGameEquip slot handlers — invoked directly to bypass click-sim hit-test problems on the equip panel.
- L438 `typedef void (__thiscall* PFN_InGameEquipOnSelectSlot)(void* panel, void* slot_btn)`
- L480 `typedef void (__thiscall* PFN_InGameEquipOnItemSelected)(void* panel, void* item_entry)`: Picker-side commit handlers.
- L481 `typedef void (__thiscall* PFN_InGameEquipOnOKPressed)(void* panel, void* btn_equip)`
- L547 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnEnterSlot) (void* panel, void* slot_btn)`: CSWGuiUpgrade (workbench upgrade.gui) slot-pick + commit chain.
- L548 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnSlotSelected) (void* panel, void* slot_btn)`
- L549 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnUpgradeSelected)(void* panel, void* item_entry)`
- L550 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnAssemble) (void* panel, void* btn_assemble)`
- L567 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnControlEntered)(void* panel, void* item_entry)`: CSWGuiUpgrade::OnControlEntered @0x006c5370 — __thiscall(panel, item_entry).
- L602 `typedef void (__thiscall* PFN_CSWGuiUpgradeShowItems)(void* panel, int visible)`: CSWGuiUpgrade::ShowItems @0x006c2f80 — __thiscall(int visible).
- L948 `typedef void (__thiscall* PFN_AddItemProperty)(void* item, CExoString* accum)`: Per-category property-block builders that GetPropertyDescription calls in fixed order (decompile-verified at 0055f340).
- L985 `typedef CExoString* (__thiscall* PFN_CExoStringCtor)(CExoString* this_)`: CExoString default constructor — __thiscall(CExoString* this).

Also 112 constants or variables at namespace scope.
