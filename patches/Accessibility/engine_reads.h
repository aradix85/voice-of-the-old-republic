// SEH-guarded read helpers for KOTOR GUI controls. Safe from hook
// handlers that may run during engine mid-teardown — every deref is
// __try-wrapped, every potentially-stale pointer is validated before use.

#pragma once

#include <cstddef>
#include <cstdint>

#include "engine_offsets.h"

namespace acc::engine {

// True iff the control had non-empty tooltip text. Hands back the raw
// (unterminated) tooltip_string pointer and length plus the id, for logging.
bool ReadControlNameFields(void* control, const char*& outTip,
                           uint32_t& outTipLen, int& outId);

// CSWGuiControl.id, the .gui "ID" property. -1 on fault, which is also the
// engine's default for a control without one.
int ReadControlId(void* control);

// An object's vtable address: 0 for a null object and on a fault. Every
// CSWGui class has its own, so it tells the class of a control and of a
// panel (a heap-allocated single-class panel has no CGuiInGame slot, and
// its vtable is its identity). Use these and not a deref of your own: a
// panel that is listed in the manager is not proven alive, the array still
// holds freed panels during teardown.
uintptr_t ReadVtable(void* object);

// The object is of the class with this vtable. False for a vtable that is
// not known in this game (0).
bool HasVtable(void* object, uintptr_t vtable);

// vtable[index](control) as __thiscall.
//
// SEH-wrapped: per-tick monitors walk panels[] and call this on every
// child control. During engine teardown (FireActivate("Play") /
// quit-confirm OK) the engine frees children synchronously while the
// panel pointer can still resolve next tick. The freed control's vtable
// slot yields garbage; without SEH the deref would crash. Faults map
// to "not this subclass" so the caller skips the stale control.
void* CallDowncast(void* control, int vtableIndex);

// CExoString at base+offset. True if non-empty and length sane.
bool ReadCExoString(void* base, size_t offset, char* outBuf, size_t bufSize);

uint32_t ReadU32(void* base, size_t offset);

// SEH-guarded single-field reads for possibly-dead engine pointers (a
// non-null control pointer is NOT proof of life on KOTOR 2). Return false
// on fault instead of crashing; use for any read through a pointer the
// engine may have freed since we captured it.
bool TryReadU32(void* base, size_t offset, uint32_t* out);
bool TryReadU16(void* base, size_t offset, uint16_t* out);
bool TryReadPtr(void* base, size_t offset, void** out);
bool TryReadFloat(void* base, size_t offset, float* out);

// SEH-wrapped TLK lookup. False on invalid strref, missing TLK pointer,
// or any fault.
bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize);

// Mirrors CSWGuiControl::DisplayToolTip @0x418a90:
//   1. tooltip_strref at +0x24 → TLK lookup.
//   2. tooltip_string at +0x28 if non-empty.
//   3. Bubble up to parent_control at +0x14 and retry.
// SEH-guarded.
bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize);

// Read directly from CAurGUIStringInternal — the engine's actually-
// rendered string. CSWGuiText::Draw reads ONLY through gui_string;
// text_params is unused at draw time. gui_string is the ground truth
// when a control has visible rendered text.
//
// guiStringPtrOffset = offset of the CAurGUIStringInternal* field
// within `control` (CSWGuiLabel 0xE4, CSWGuiButton 0x168).
//
// We vtable-check guiString before dereffing at +0x14: chargen Class
// buttons reach our chain in a state where +0x168 transiently holds a
// non-null garbage pointer (engine writes invalid between reads). The
// vtable check skips garbage without relying on SEH to absorb the AV
// — under /GS the AV unwind can fastfail with c0000409 instead of
// being absorbed. SEH stays as defense for unmapped-memory faults.
bool ReadGuiString(void* control, size_t guiStringPtrOffset,
                   char* outBuf, size_t bufSize);

// Inline CExoString, then TLK strref fallback.
bool ExtractTextOrStrRef(void* control,
                         size_t cexoOffset, size_t strRefOffset,
                         char* outBuf, size_t bufSize);

// Try every text path:
//   1. CAurGUIStringInternal at gui_string (preferred — reflects render).
//   2. Inline CExoString (text_params.text).
//   3. strref → TLK lookup.
//   4. text_object indirection (defensive fallback).
//
// gui_string first because overridden subclasses (CSWGuiInGameMenu's
// icon labels are the canonical case) leave the other paths empty.
bool ExtractTextOrStrRefIndirect(void* control,
                                 size_t cexoOffset, size_t strRefOffset,
                                 size_t textObjectOffset,
                                 char* outBuf, size_t bufSize);

// Read a CSWGuiLabel's rendered text — engine's resolved gui_string path
// with the strref / CExoString / text_object fallback chain (i.e. the
// label-specific shape of ExtractTextOrStrRefIndirect). SEH-guarded;
// outBuf = "" on miss. Pass a label-control pointer (NOT a panel — for
// the (panel, offset) convenience form use ReadLabelTextAt below).
bool ReadLabelText(void* label, char* outBuf, size_t bufSize);

// Convenience: panel + label-field offset, for the common case where a
// label is embedded in the panel struct at a known field.
inline bool ReadLabelTextAt(void* panel, size_t offset,
                            char* outBuf, size_t bufSize) {
    if (!panel) {
        if (outBuf && bufSize > 0) outBuf[0] = '\0';
        return false;
    }
    return ReadLabelText(
        reinterpret_cast<unsigned char*>(panel) + offset, outBuf, bufSize);
}

// Read a CSWGuiButton's rendered text — same two-step shape against the
// button's own field offsets (kButton{GuiStringPtr,Text,StrRef,TextObject}
// Offset). SEH-guarded; outBuf = "" on miss.
bool ReadButtonText(void* button, char* outBuf, size_t bufSize);

// Used by ExtractAnnounceableText (which field reads) and
// IsChainNavigable (where chain nav can land).
bool IsToggle(void* control);
bool IsSlider(void* control);
bool IsListBox(void* control);
bool IsEditbox(void* control);

// CSWGuiControl.bit_flags read, SEH-guarded. 0 on fault — callers treat
// that as "not visible / not usable", never the wrong direction.
uint32_t ReadControlBitFlags(void* control);

// bit_flags & kControlVisibleBit — the engine's own "shown" bit, which
// panels with several stacked listboxes (store, crafting, messages) flip to
// pick the one on screen. False on fault.
bool IsControlVisible(void* control);

// A CSWGuiListBox's rows. Every reader answers "nothing" for a null list
// box, on a fault and for an implausible size; none reads through a row.
//
// The row count (the size of the controls array), -1 when unreadable.
int ReadListBoxRowCount(void* listBox);
// Row `index`, nullptr out of range.
void* ReadListBoxRow(void* listBox, int index);
// selection_index; -1 is also the engine's "no selection".
int ReadListBoxSelection(void* listBox);
// The selected row, nullptr without a selection.
void* ReadListBoxSelectedRow(void* listBox);
// The index of `row` among the rows, -1 when it is not one.
int FindListBoxRow(void* listBox, void* row);

// A CSWGuiEditbox's text. The length comes from strnlen: the engine does
// not keep the CExoString length in sync on Backspace, but it terminates
// the buffer on every change. True with "" for a null buffer; false on
// fault.
bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen);

// CSWGuiButtonToggle.field2_0x1c8 bit 0 — HandleInputEvent XORs on
// activate, Draw branches on (& 1) for which border to render.
bool ReadToggleState(void* toggle);

// vtable[0..7] dump. Each CSWGui subclass has a unique vtable address,
// so this correlates unknown controls back to specific classes via SARIF.
void DumpControlVtable(void* control, char* out, size_t outSize);

// Convert a client-side object id to its server-side id via
// CServerExoApp::ClientToServerObjectId. Returns 0 on null/invalid input or
// any resolution failure. Pairs with engine_area::ResolveServerObjectHandle
// to cross from a client id to the server CSWSObject*.
uint32_t ClientToServerObjectId(uint32_t clientHandle);

// Client-side handles (the kind in row+0x1c4 on CSWGuiStoreItemEntry,
// CSWGuiInGameItemEntry in Container/Equip-picker) → CSWSItem*. Walks
// AppManager → CServerExoApp + ClientToServer + GetItemByGameObjectID
// with SEH per hop. Null on any failure.
void* ResolveItemFromClientHandle(uint32_t clientHandle);

// Multi-line property description (damage / range / on-hit / defence /
// base description). Engine allocates a heap c_string per call which
// we deliberately leak (CRT-mismatch — same pattern as LookupTlk).
bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize);

// The four screen-reader navigation blocks of an item's property description,
// reconstructed by calling the engine's own per-category builders (the same
// ones GetPropertyDescription chains) into separate accumulators. Each field is
// an empty string ("") when that category has no content for this item; callers
// skip empties. Locale-clean (no string parsing). See BuildItemDescriptionBlocks.
struct ItemDescriptionBlocks {
    char tags[2048];        // required feats / weapon proficiency / restriction
    char values[2048];      // damage, range, crit, on-hit, attack & defence bonus
    char properties[4096];  // "Special:" special / enchant-style modifications
    char description[4096]; // RPG flavour text
};

// Build the four blocks for `item` (a CSWSItem*). Returns false on null/fault or
// when even GetPropertyDescription yields nothing. The per-category accumulators
// are deliberately leaked (CRT-mismatch — same rule as ReadItemPropertyDescription).
bool BuildItemDescriptionBlocks(void* item, ItemDescriptionBlocks* out);

// Workbench upgrade slot → installed-mod item. Given the CSWGuiUpgrade panel
// and a slot button control (upgrade.gui IDs 12..18), reads the slot button's
// custom_value and returns the installed mod CSWSItem* at
// field35_0x2f74[custom_value], or nullptr when the slot is empty / inputs
// invalid. The returned pointer is a real CSWSItem* (engine-constructed from
// the mod template) and is safe to pass to ReadItemPropertyDescription /
// ExtractTextOrStrRef(+0x280). SEH-guarded. See kUpgradeSlotInstalledItemsOff.
void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl);

// Layout of the workbench mod-picker (LB_ITEMS) for the currently-open slot.
// The picker only appears for SABER slots (non-saber slots install/remove
// their single compatible mod directly). The row layout depends on slot type:
//   * Saber color slot (active slot custom_value == 1): row 0 is the
//     currently-set color crystal; every row is a real selectable choice and
//     there is no remove entry → minSel 0, installedRow 0.
//   * Saber power slot (custom_value != 1): row 0 is the 0x7f000000 "remove
//     upgrade" entry (hidden from nav → minSel 1); row 1 is the installed
//     crystal when the slot is occupied (installedRow 1), else installedRow -1.
// `valid` is false when the active slot can't be resolved (picker not open).
struct WorkbenchPickerInfo {
    bool valid        = false;
    bool isColorSlot  = false;
    int  minSel       = 0;   // first navigable LB_ITEMS row
    int  installedRow = -1;  // LB_ITEMS index of the installed/current crystal
};
WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel);

// CSWGuiInterfaceAction.action_id → human-readable description.
// Dispatches on the high-nibble tag stamped by each entry-creator:
//   0x1xxxxxxx feat        — CSWRules::GetFeat → CSWFeat::GetDescriptionText
//   0x2xxxxxxx force power — CSWSpellArray::GetSpell → spell_description
//                            strref → LookupTlk
//   0x4xxxxxxx item        — CServerExoApp::GetItemByGameObjectID →
//                            CSWSItem::GetPropertyDescription
// All other categories (attack verbs, door open/unlock, computer hack,
// etc.) return false — those are plain verbs the engine never surfaces
// extra text for; caller falls back to localised "no description
// available" cue.
//
// Tag encodings decoded via Ghidra decompile of CSWCCreature::
// CreateUsableItemEntry @0x006193a0, ClassInfo::GetMenuInfo @0x0064a870,
// and EnableFeatForMenu @0x00618a30.
bool ResolveActionDescriptionFromActionId(uint32_t actionId,
                                          char* outBuf, size_t bufSize);

// Returns:
//   > 1   stackable; caller speaks the count suffix.
//     1   single — caller stays silent (saying "1" is noise).
//     0   not an item-entry row, unresolved, or store infinite stock.
//
// Engine renders the count overlay when stack_size >= 2 (SetItem
// confirmed for both inventory + store row vtables).
int ReadItemRowStackCount(void* rowControl);

// Charge readers — for limited-use consumables that carry a current /
// max charge count (CSWSItem.charges / max_charges). Charged items can't
// stack, so a positive result here never coincides with a stack suffix.
//   ReadItemCharges(item)        : >= 0 current charges when max_charges>0,
//                                  else -1 (not a charged item / fault).
//   ReadItemRowCharges(rowCtrl)  : same, resolved from an inventory/store
//                                  row control; -1 when not an item row.
int ReadItemCharges(void* item);
int ReadItemRowCharges(void* rowControl);

// Name an item row from the ITEM it points at, not from its own caption.
// The engine normally writes the localised name into the row control and
// every extractor reads it there; a row whose caption comes up empty used
// to fall through to the spatial sibling-label search, which for a listbox
// row can only ever match something on the panel behind it (KOTOR 2
// announced one inventory item as the screen title "Inventory" -
// userlogs/daninventoryequipmentbroken077, three separate visits).
// Resolves the row's obj_id through GetObjectDisplayNameByHandle, so it
// answers for both row vtables (inventory/container and store) and for
// either handle namespace. False when the control is not an item row, the
// handle is a sentinel, or the name does not resolve.
bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize);

// Text of the CExoLocString at base+offset from a substring that is not
// English, for strings the English game cannot resolve. The engine's lookup
// (CExoLocString::GetString) tries only the game language's substring and
// then the TLK strref; it never falls back across languages, so a mod item
// with only a foreign or malformed substring (mods write ids such as
// 0xFFFFFFFF, which no language lookup matches) reads as empty.
//
// True only when no English substring exists and the first substring has
// text; false whenever an English substring exists, so the engine's own
// text is never overridden.
bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset,
                                char* outBuf, size_t bufSize);

// Raw stack_size for a resolved CSWSItem* (0 on fault / infinite stock).
// The row/handle wrappers above are preferred for menu rows; this is for
// callers that already hold the item* (store suffix, action-menu items).
int ReadItemStack(void* item);

// Resolve the CSWSItem* behind an item-tagged CSWGuiInterfaceAction
// action_id (high nibble 0x4xxxxxxx, low 28 bits = server game-object id).
// Returns nullptr for non-item tags / unresolved handles. Lets the action
// menu read stack + charges for its "Items" entries the same way
// inventory rows do.
void* ItemFromActionId(uint32_t actionId);

// The rules rows of a feat (CSWRules::GetFeat) and of a Force power
// (CSWSpellArray::GetSpell), or nullptr.
void* GetFeatRow(uint16_t featIdx);
void* GetSpellRow(int spellId);

// The TLK text of the strref at `offset` of such a row; false when there is
// none.
bool ReadRowStrRefText(void* row, size_t offset, char* outBuf, size_t bufSize);

// CSWGuiInGameItemEntry rows (Inventory + Container loot listbox).
// Store rows excluded — they get a price+stock suffix from
// menus_store::AnnounceChainStepSuffix instead.
bool IsInventoryItemRow(void* control);

// Read a client creature's live Force pool. Walks CSWCCreature+0x2f8 →
// CSWCLevelUpStats (embeds CSWCCreatureStats at +0), then reads
// max_force_points @+0x11e and the live current pool as the sum of the two
// shorts at +0x122/+0x124 (matching CSWGuiInGameCharacter::SetStats, which
// reads this client struct; the +0x120 "force_points" field is a static base,
// not the live current).
// Same struct as ReadClientCreatureHitPoints. Returns false
// on null/fault. *outMax == 0 is a VALID result meaning "not a Force
// user" (non-Jedi classes, droids) — callers gate the FP display on
// *outMax > 0, NOT on the return value. `clientCreature` is a
// CSWCCreature* (GetClientLeader(), or the charsheet panel's displayed-
// creature pointer at +0x59e4).
bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax);

// A client creature's live hit points, from the stats block its character
// sheet renders (CSWCCreatureStats: +0x4c pregame_current_hp, +0x4e
// max_hit_points; +0x48 hit_points is the base value, not the live one).
// Right for every party member, where the server accessor
// CSWSCreature::GetMaxHitPoints(1) gates on is_pc and returns garbage for
// the others. False on null / fault, outputs untouched.
bool ReadClientCreatureHitPoints(void* clientCreature, int* outCur, int* outMax);

// Icon ids of a server creature's effect-icon row (CSWSCreature.effect_icons:
// the icons its portrait shows, priority-sorted, one entry per icon id).
// Creatures only. Returns the count written to outIds; 0 on fault, and when
// the row claims more than cap entries.
int ReadCreatureEffectIconIds(void* serverCreature, int* outIds, int cap);

// EFFECT_TYPES of the first cap effects in CSWSObject.effects, in list order
// (a type can repeat). Returns the count written; a fault mid-walk keeps
// what was read.
int ReadObjectEffectTypes(void* serverObject, int* outTypes, int cap);

// The 0..5 visible wound-state bucket for a server object (0 healthy,
// 1 light, 2 wounded, 3 badly wounded, 4 dying, 5 dead), -1 when
// unreadable. KOTOR 1: CSWSObject::GetDamageLevel engine accessor.
// KOTOR 2: that accessor has no located twin, so this replicates its
// decompiled ratio bucketing from the two Pick'd HP accessors instead
// — same inputs (current+temporary HP over GetMaxHitPoints(1)), same
// thresholds (95/75/50/25/0%). Shared by combat_query's target brief
// and the examine view's condition row.
int ReadObjectDamageLevel(void* serverObject);

// Current hit points of a server object (CSWSObject +0xe0, signed short),
// -1 on null / fault. Any object kind — creatures, placeables, doors.
int ReadObjectCurrentHitPoints(void* serverObject);

// Maximum hit points of a server creature (GetMaxHitPoints(1), the same call
// ReadObjectDamageLevel's KOTOR 2 path makes), -1 on null / fault. Used by
// the local bridge's party readout.
int ReadCreatureMaxHitPoints(void* serverCreature);

// Current hit points through the engine accessor GetCurrentHitPoints(0)
// (current plus temporary), -1 on null / fault. The raw +0xe0 field read by
// ReadObjectCurrentHitPoints is not the live value for the player creature.
int ReadCreatureCurrentHitPoints(void* serverCreature);

// True when a server object carries the plot flag (indestructible). Static
// placeables and doors also read true: both games' loaders set the flag for
// them. False on null / fault.
bool IsPlotObject(void* serverObject);

}  // namespace acc::engine
