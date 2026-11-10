# engine_offsets_addresses.h (1071 lines)

Source: `patches/Accessibility/engine_offsets_addresses.h`

Engine executable addresses: .text functions, vtables, and .data globals.

Part of the engine_offsets.h family - see engine_offsets_types.h for the split rationale and the full file list.

Two upstream AddressDatabase tables live here and they behave differently:

functions and vtables (.text / .rdata) - EVERY one goes through acc::addr::R(). The Allard Russian build is the same BioWare source relinked, so these move by -320..+640 bytes each, per function and not monotonically (engine_rebase.h). Of the 214 addresses `kdev sigscan` resolved against that build, zero kept their reference value. Declaring a .text address here without R() therefore does not degrade gracefully: it calls into the middle of an unrelated function.

global pointers (.data) - never through R(): .data is byte-stable across the KOTOR 1 builds, so it is not rebased, and feeding it to R() would resolve it to 0 on the relink. They have their own section at the bottom of this file.

Every constant here is `const uintptr_t`, not `constexpr` - the marker is a runtime call resolved at load time against the detected game.

KOTOR 2. Each address is in one of two states, and there is no third: a recompile relocates everything, so a KOTOR 1 address is NEVER also valid in KOTOR 2 (unlike a struct offset, which often is).

.text / .rdata:  R(k1)                 - KOTOR 2 value not yet found -> 0 Pick(k1, k2)          - known .data:           TodoGlobal(k1)        - not yet found -> 0 PickGlobal(k1, k2)    - known

A .data pointer must never be left bare `constexpr`: that hands out the KOTOR 1 address on KOTOR 2, which is a plausible-looking pointer into unrelated data rather than an obvious failure.

## Declarations (in source order)

- L381 `typedef void (__thiscall* PFN_CSWGuiListBoxSetSelectedControl)(void* listbox, int index, int playSound)`: CSWGuiListBox::SetSelectedControl @0x0041c040 — __thiscall(listbox, index, playSound).
- L403 `typedef CExoString* (__thiscall* PFN_GetSimpleString)(void* this_, CExoString* out, uint32_t strref)`: CTlkTable::GetSimpleString — resolves a TLK str_ref to a localized string.
- L452 `typedef void (__thiscall* PFN_InGameEquipOnEnterSlot)(void* panel, void* slot_btn)`: CSWGuiInGameEquip slot handlers — invoked directly to bypass click-sim hit-test problems on the equip panel.
- L453 `typedef void (__thiscall* PFN_InGameEquipOnSelectSlot)(void* panel, void* slot_btn)`
- L495 `typedef void (__thiscall* PFN_InGameEquipOnItemSelected)(void* panel, void* item_entry)`: Picker-side commit handlers.
- L496 `typedef void (__thiscall* PFN_InGameEquipOnOKPressed)(void* panel, void* btn_equip)`
- L562 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnEnterSlot) (void* panel, void* slot_btn)`: CSWGuiUpgrade (workbench upgrade.gui) slot-pick + commit chain.
- L563 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnSlotSelected) (void* panel, void* slot_btn)`
- L564 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnUpgradeSelected)(void* panel, void* item_entry)`
- L565 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnAssemble) (void* panel, void* btn_assemble)`
- L582 `typedef void (__thiscall* PFN_CSWGuiUpgradeOnControlEntered)(void* panel, void* item_entry)`: CSWGuiUpgrade::OnControlEntered @0x006c5370 — __thiscall(panel, item_entry).
- L617 `typedef void (__thiscall* PFN_CSWGuiUpgradeShowItems)(void* panel, int visible)`: CSWGuiUpgrade::ShowItems @0x006c2f80 — __thiscall(int visible).
- L975 `typedef void (__thiscall* PFN_AddItemProperty)(void* item, CExoString* accum)`: Per-category property-block builders that GetPropertyDescription calls in fixed order (decompile-verified at 0055f340).
- L1012 `typedef CExoString* (__thiscall* PFN_CExoStringCtor)(CExoString* this_)`: CExoString default constructor — __thiscall(CExoString* this).

Also 112 constants or variables at namespace scope.
