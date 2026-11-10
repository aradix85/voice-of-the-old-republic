// Internal to the engine_radial module (engine_radial*.cpp); no other file includes
// it. The public interface is engine_radial.h. Here: what the files share.

#pragma once

#include "engine_radial.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>

#include "engine_rebase.h"
#include "engine_offsets_select.h"

namespace acc::engine_radial {

// CExoArrayList<CSWGuiInterfaceAction> action_lists[3] occupy TAM +0x00
// through TAM +0x24 (3 * 0x0C). We only need the size field, which lives
// at +0x04 within each CExoArrayList.
// K2 witnessed in the K2 TAM::DoTargetAction (0x007445D0): lists read as
// [tam + row*0xc] data / +4 size — layout unchanged.
const size_t kTamActionListsOffset    = acc::off::Same(0x00);
const size_t kActionListSizeOffset    = acc::off::Same(0x04);
const size_t kActionListStride        = acc::off::Same(0x0C);

// CSWGuiMainInterfaceAction stride / row offset within TAM. target_actions[3]
// starts at +0x54 (after action_lists[3] @+0x00 + field1[12] @+0x24).
// K2 witnessed in the K2 TAM ctor (0x00742150): eh-vector of 3 rows at
// this+0x54 with stride 0x750 (row class grew with its sub-controls).
const size_t kTamTargetActionsOffset  = acc::off::Same(0x54);
const size_t kTargetActionStride      = acc::off::Pick(0x71C, 0x750);

// Within a CSWGuiMainInterfaceAction, action_button is the first member at
// offset 0. is_action lives at +0x718 (one int before the next stride
// boundary at 0x71C). Embedded buttons: action_label at +0x1C4
// (CSWGuiButton size 0x1C4), up_button +0x388, down_button +0x54C, then
// field4 +0x710, field5 +0x714, is_action +0x718.
// K2 witnessed in the K2 CSWGuiMainInterfaceAction ctor (0x00741420): four
// sub-control ctors at this+0 / +0x1d0 / +0x3a0 / +0x570 in declaration
// order (button, label, up, down — each control grew 0xC).
const size_t kRowActionButtonOffset   = acc::off::Same(0x000);
const size_t kRowActionLabelOffset    = acc::off::Pick(0x1C4, 0x1D0);
const size_t kRowUpButtonOffset       = acc::off::Pick(0x388, 0x3A0);
const size_t kRowDownButtonOffset     = acc::off::Pick(0x54C, 0x570);
// K2 field4/field5 witnessed in the refactored row-flag setter (0x00741780):
// `[row+0x744] byte = 2; [row+0x740] float = 0.1f` — K1's exact write pair.
const size_t kRowField4Offset         = acc::off::Pick(0x710, 0x740);
const size_t kRowField5Offset         = acc::off::Pick(0x714, 0x744);
// K2: the row's shown-state flag byte is at +0x74d (witnessed in the K2
// row Show 0x00741660: `[row+0x74d] = (hide != 0)`; ctor inits it 1).
// Diag-only int32 read — anchored at +0x74c so the 4-byte read stays
// inside the 0x750 row; the flag appears in byte 1 of the logged value.
const size_t kRowIsActionOffset       = acc::off::Pick(0x718, 0x74c);

// CSWGuiInterfaceAction stride/offsets — same as engine_picker uses on
// the descriptor read. Re-stated here so the wide-diagnostic peek into
// action_lists[r].data[0] doesn't depend on the picker module.
// K2: id/target/icon/flags positions unchanged (witnessed across the K2
// GetDefaultActions descriptor writes + Do*Action reads); label is the
// leading 8-byte CExoString (+0..+7 — id at +8 pins it). Stride grew
// 0x38 -> 0x3C, tail growth only.
const size_t kIfActionLabelOffset    = acc::off::Same(0x00);  // CExoString
const size_t kIfActionIdOffset       = acc::off::Same(0x08);  // ulong
const size_t kIfActionTargetOffset   = acc::off::Same(0x1c);  // ulong
const size_t kIfActionIconOffset     = acc::off::Same(0x20);  // CResRef (16B)

// CClientExoApp::GetGameObject @ 0x005ED580 — same address engine_area /
// engine_picker use. (this, handle) -> CGameObject*. K2 twin 0x0073F4D0
// (see kAddrCClientExoAppGetGameObject in engine_area.h for the witness);
// kept as Pick here purely for consistency — this module stays K1-gated
// until the Batch 6 triage.
const uintptr_t kAddrCClientGetGameObject = acc::addr::Pick(0x005ED580, 0x0073F4D0);

// GameObjectMethods vtable indices we care about for runtime-class probing.
// Layout from swkotor.exe.h:14638. Each entry is one __thiscall pointer
// (4 bytes on x86); offsets are byte offsets into the vtable.
//   [0] _destructor
//   [1] SetId
//   [2] ResetUpdateTimes
//   [3] AsSWCObject     (+0x0C)
//   [4] AsSWSObject     (+0x10)
//   [5] AsSWCDoor       (+0x14)
//   [6] AsSWSDoor       (+0x18)
//   [10] AsSWCCreature  (+0x28)
//   [14] AsSWCTrigger   (+0x38)
//   [18] AsSWCPlaceable (+0x48)
// K2 slots witnessed in the K2 GetDefaultActions (0x007B40F0): AsSWCDoor
// +0x14 (unchanged), AsSWCCreature +0x2c, AsSWCTrigger +0x3c, AsSWCPlaceable
// +0x4c — one virtual inserted below +0x14, shifting the later three by 4.
const size_t kVtableAsSWCDoorOffset      = acc::off::Same(0x14);
const size_t kVtableAsSWCCreatureOffset  = acc::off::Pick(0x28, 0x2c);
const size_t kVtableAsSWCTriggerOffset   = acc::off::Pick(0x38, 0x3c);
const size_t kVtableAsSWCPlaceableOffset = acc::off::Pick(0x48, 0x4c);

// DoTargetAction takes (this, int row) but does ret 8 (Ghidra
// BYTES_PURGED="8"): it purges a second dword it doesn't use. Typed without it,
// the call under-pushes by 4 and corrupts the caller's frame (the
// SetMainInterfaceTarget class of bug); push a matching pad.
typedef void (__thiscall* PFN_DoTargetAction)(void* this_, int row, int pad);
typedef void* (__thiscall* PFN_GetGameObject)(void* exoApp, uint32_t handle);
typedef void* (__thiscall* PFN_AsClass)(void* gameObject);
typedef bool (__thiscall* PFN_GetCanUseSkill)(void* stats, uint16_t skillIdx);

// Helpers shared across the files, by the file that defines them.

// engine_radial.cpp
bool ReadInt32(void* base, size_t offset, int32_t* out);
bool ReadGuiStringLocal(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize);
bool ReadCExoStringLocal(void* base, size_t offset, char* outBuf, size_t bufSize);
void* CallVtableAsClass(void* gameObject, size_t vtableOffset);

}  // namespace acc::engine_radial
