// The menus feature's private seam: helpers and state the menus*.cpp files
// share, plus the panel-member resolvers peek_description borrows. The
// public interface is menus.h.

#pragma once

#include <cstddef>
#include <cstdint>
#include "engine_offsets_select.h"

namespace acc::menus::detail {

// Buffer size every announce path allocates for a control's spoken text,
// sized so nothing the game renders is clipped: the longest real string, a
// chargen class name plus its description, runs a bit over 250 characters in
// German. EmitText truncates beyond it.
constexpr size_t kAnnounceTextMax = 1024;

bool IsChainNavigable(void* control);
bool IsClassSelectionIcon(void* panel, void* control);

// True for a listbox container with more than one row. SetActiveControl on
// one is the engine's panel-open auto-focus, not the user navigating: rows
// are reached through the listbox specs and the chain.
bool IsMultiRowListBox(void* control);

// Chargen class selection: the (panel, icon) to spoken text cache.
const char* ClassLabelCacheLookup(void* panel, void* icon);
void ClassLabelCacheStore(void* panel, void* icon, const char* text);

// Compose "class name" + the engine's class blurb into one announce string.
// Must be called at the moment class_label is read — both labels hold the
// NEXT icon's text by the time a chain step announces this one.
void ComposeClassAnnounce(void* panel, const char* name,
                          char* outBuf, size_t bufSize);

// Class-description placeholder discrimination. classsel.gui authors
// LBL_DESC with a placeholder that the engine replaces on hover; Capture
// snapshots the authored text at panel-walk time (before any hover) and
// IsEngineWritten answers "is this the engine's class description, or still
// the .gui default?" for whatever is in the label now.
void ClassDescBaselineCapture(void* panel, const char* text);
bool ClassDescIsEngineWritten(void* panel, const char* text);

// Convert a distance measured in .gui authoring units into the units
// GetControlCenter returns on the running game. KOTOR 1 ships one .gui per
// resolution and hands back the authored extents verbatim; KOTOR 2 stretches
// its single 800x600 layout to the window and stores the result in the
// extents, so at 2880x1800 every distance is 3.6x the authored one. Every
// bare pixel threshold here was measured off a KOTOR 1 .gui file and goes
// through this. Returns `guiPx` unchanged on KOTOR 1, and on KOTOR 2
// whenever the window is unreadable, so a failed read never shrinks a
// threshold.
int ScaleGuiThresholdPx(int guiPx);

// Screen centre of a control's extent; false on a degenerate extent or a
// freed control.
bool GetControlCenter(void* control, int& outCx, int& outCy);

// A child control of `panel` by its .gui id (the +0x50 field).
void* FindControlById(void* panel, int id);

// The CSWGuiSaveLoad panel, by its vtable. SEH-safe on stale pointers.
bool IsSaveLoadPanel(void* panel);

// A CExoString field of a save-row entry, by byte offset; nullptr when empty.
const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset);

// Result of a single arrow-key step on a listbox cursor. Filled by
// DriveListBoxSelection; caller reads `row` to announce, `newSel`/`rowCount`
// for index reporting, and `oldSel == newSel` to detect a boundary clamp
// (per-tick monitors that watch selection_index changes won't fire on a
// no-op step, so callers using a monitor announce inline only on clamp).
struct ListBoxNavResult {
    short oldSel;     // selection_index before the step
    short newSel;     // selection_index after the step
    int   rowCount;   // listbox.controls.size
    void* row;        // row pointer at newSel (nullptr iff rowCount == 0)
};

// Direction of cursor motion driven by DriveListBoxSelection.
//   * StepUp / StepDown: ±1 with boundary clamp (Nav-Up / Nav-Down).
//   * JumpFirst / JumpLast: absolute jump to minSel / rowCount-1
//     (Home / End). oldSel == newSel still means "already at the boundary".
enum class ListBoxNavOp { StepUp, StepDown, JumpFirst, JumpLast };

// Drive a CSWGuiListBox cursor in response to Nav-Up / Nav-Down / Home / End.
// Pure listbox-side effect: writes selection_index + scrolls
// top_visible_index to keep the new selection visible. Does NOT call
// SetSelectedControl, so the engine's onSelectionChanged callback won't
// run. minSel = first valid selection (0 normally, 1 for equip-picker
// LB_ITEMS to skip the protoitem template at row 0). Returns false iff
// listbox is null, unreadable or empty.
bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel,
                           ListBoxNavResult& out);

// Same contract as DriveListBoxSelection, but drives the engine's own
// CSWGuiListBox::SetSelectedControl instead of writing selection_index raw.
// SetSelectedControl sets the row's real highlight state, plays the GUI select
// sound, and scrolls the page natively (OrganizeControls) — so multipage lists
// work the engine's way and the selection survives the per-frame hover-select
// that overwrites a raw index write. Callers that use this MUST also keep the
// mouse cursor off the list while armed (see the picker arm path), otherwise
// the engine's hover handler re-selects the row under the cursor every frame.
bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel,
                                 ListBoxNavResult& out);

// Warp the OS cursor to the empty top-left corner of the screen so the engine's
// per-frame hover-select (CSWGuiListBox::HandleMouseMove -> HitCheckMouseLocal
// -> SetSelectedControl) finds no row under it and stops overwriting the
// keyboard-driven selection_index writes DriveListBoxSelection makes. Any panel
// that drives a listbox from the keyboard must park the cursor while it is up.
//
// MUST be called from a per-tick monitor (Update tick), never from the input
// hook — MoveMouseToPosition recurses through the hover pipeline. `tag` is the
// caller's acclog tag. The corner has to clear the camera edge-turn band; see
// menus_monitors.cpp.
bool ParkCursorToCorner(const char* tag);

// Queue activation of a control resolved through a panel-member resolver.
// Used by every select-then-confirm panel to commit its Enter/Esc. Returns
// false on debounce or a null target — the caller still consumes the keypress
// so the engine's stale activeControl can't take over.
bool QueueControlActivate(void* target, const char* logPrefix);

}  // namespace acc::menus::detail

// The focused panel, written only by OnSetActiveControl in menus.cpp.
extern void* g_currentPanel;

// Liveness-filtered view of g_currentPanel — returns it only while it is
// still in the manager's panels[], nullptr once the engine has freed it.
//
// g_currentPanel is a raw cached pointer with no invalidation hook, and
// OnSetActiveControl (its only writer) is SUPPRESSED for the whole duration
// of a save-load / module load. So every area transition leaves it pointing
// at a panel the module teardown freed, and it stays that way until the next
// ordinary focus event. The freed block is promptly reused, so panel+0x20 /
// +0x24 (CExoArrayList data + size) read back as whatever now lives there —
// non-null, plausibly-sized, and data[0] takes an AV. Same failure the chain
// side already guards with ValidateChainPanel; this is its shared form.
//
// Use this at every site that DEREFERENCES the panel. Pointer comparisons
// (g_chainPanel != g_currentPanel and friends) may read the raw global.
void* CurrentPanelIfLive();

// State owned by menus.cpp and shared with the other menus files:
//
//   * g_drilledIntoSubScreen — arrow-nav retarget flag. Set/cleared by the
//     drill router + Esc gate in menus_dispatch.cpp, published to the rest
//     of the mod through menus.h's IsDrilledIntoSubScreen accessor.
//   * s_synthesizedNav — true while PollHomeEndKeys (menus.cpp) is
//     synthesising a call into OnHandleInputEvent (menus_dispatch.cpp), so
//     the press/release pair tracker there skips the synthetic press.
//   * s_pendingAnnounce{Panel,Control} — the last-write-wins focus slot.
//     Written by AnnounceNewFocusedControl (menus_focus.cpp), drained once
//     per tick by DrainPendingAnnounce (menus.cpp).
extern bool g_drilledIntoSubScreen;

namespace acc::menus {
extern bool  s_synthesizedNav;
extern void* s_pendingAnnouncePanel;
extern void* s_pendingAnnounceControl;
}  // namespace acc::menus

namespace acc::menus::detail {

// Equip-panel slot identity from the engine's embedded control arrays
// (kEquipPanelSlotButtons/LabelsOffset). Pure pointer arithmetic against the
// panel, no control deref, so no SEH needed.
//
// Returns the 0-based slot index shared by BOTH games' engine order:
//   0 weapL, 1 weapR, 2 head, 3 armL, 4 armR, 5 body, 6 hands,
//   7 implant, 8 belt; KOTOR 2 appends 9 weapL2, 10 weapR2.
// -1 when the control is not one of this panel's slot buttons (/labels).
// The matching per-slot equipped-item handle lives at
// panel + kEquipPanelSlotItemIdsOffset + 4*index.
int EquipSlotIndexFromButton(void* panel, void* control);
int EquipSlotIndexFromControl(void* panel, void* control);  // button OR label

// Slots in the engine's arrays for the running game (9 on K1, 11 on K2).
inline int EquipSlotCount() { return acc::game::IsKotor2() ? 11 : 9; }

// Slot-button membership on the workbench upgrade panel via its embedded
// button run (K1: 7 from +0x64, K2: 9 from +0x7a8). Returns the array index
// or -1; membership only — the slot's meaning stays in the button's
// custom_value (per bank), which LookupUpgradeSlotType and
// GetWorkbenchSlotInstalledItem read.
int UpgradeSlotIndexFromButton(void* panel, void* control);

// Panel-member resolvers. Each returns the control the panel's constructor
// binds to that member (the k<Panel>*Offset fields in
// engine_offsets_fields.h), so a renumbered .gui file cannot misdirect it.
// The control's historical .gui id stays as a tripwire: when it resolves to
// a different control, one "GuiIdMismatch" line per session is logged. A
// resolver answers nullptr where its offset is unported or poisoned, i.e. on
// a game without that panel or control. Callers identify the panel first.

// The equipment screen (equip / equip_p).
void* EquipPanelItemsListBox(void* panel);    // LB_ITEMS
void* EquipPanelEquipButton(void* panel);     // BTN_EQUIP
void* EquipPanelBackButton(void* panel);      // BTN_BACK

// CSWGuiUpgrade, the workbench upgrade screen (upgrade / upgrade_p).
void* UpgradePanelItemsListBox(void* panel);    // LB_ITEMS
void* UpgradePanelAssembleButton(void* panel);  // BTN_ASSEMBLE
void* UpgradePanelBackButton(void* panel);      // BTN_BACK

// CSWGuiSaveLoad (saveload / saveload_p).
void* SaveLoadPanelGamesListBox(void* panel);   // LB_GAMES
void* SaveLoadPanelActionButton(void* panel);   // BTN_SAVELOAD (Load/Save)
void* SaveLoadPanelBackButton(void* panel);     // BTN_BACK

// KOTOR 2 workbench and crafting; nullptr on KOTOR 1. UpgradeSelPanel*
// address CSWGuiUpgradeSelection (upgradesel_p, the workbench top screen
// with the inline item list). CraftPanel* address CSWGuiCreateItem
// (component_p) and CSWGuiCreateMedicalItem (chemical_p), which lay the same
// controls out at different offsets, and answer nullptr for any other panel.
void* UpgradeSelPanelListBox(void* panel);        // LB_UPGRADELIST
void* UpgradeSelPanelUpgradeButton(void* panel);  // BTN_UPGRADEITEMS
void* UpgradeSelPanelTitleLabel(void* panel);     // LBL_TITLE
void* CraftPanelShopListBox(void* panel);         // LB_SHOPITEMS (create view)
void* CraftPanelInvListBox(void* panel);          // LB_INVITEMS (breakdown view)
void* CraftPanelAcceptButton(void* panel);        // BTN_Accept
void* CraftPanelExamineButton(void* panel);       // BTN_Examine (view flip)
void* CraftPanelTitleLabel(void* panel);          // LBL_TITLE

// CSWGuiPowersLevelUp (pwrlvlup / pwrlvlup_p).
void* PowersPanelPowersListBox(void* panel);      // LB_POWERS (skill-flow rows)
void* PowersPanelDescListBox(void* panel);        // LB_DESC
void* PowersPanelPowerLabel(void* panel);         // LBL_POWER (selected name)
void* PowersPanelSubTitleLabel(void* panel);      // SUB_TITLE_LBL (title)
void* PowersPanelRecommendedButton(void* panel);  // RECOMMENDED_BTN
void* PowersPanelAcceptButton(void* panel);       // ACCEPT_BTN
void* PowersPanelBackButton(void* panel);         // BACK_BTN

// CSWGuiContainer (container / container_p).
void* ContainerPanelItemsListBox(void* panel);    // LB_ITEMS (loot rows)
void* ContainerPanelOkButton(void* panel);        // BTN_OK (take all)
void* ContainerPanelCancelButton(void* panel);    // BTN_CANCEL
void* ContainerPanelGiveItemsButton(void* panel); // BTN_GIVEITEMS (give mode)

// CSWGuiFeatsCharGen (ftchrgen / ftchrgen_p).
void* FeatsPanelAcceptButton(void* panel);       // BTN_ACCEPT ("OK")
void* FeatsPanelBackButton(void* panel);         // BTN_BACK ("Cancel")
void* FeatsPanelRecommendedButton(void* panel);  // BTN_RECOMMENDED

// CSWGuiSkillInfoBox (skillinfo / skillinfo_p), the granted-feats /
// level-up info popup.
void* SkillInfoBoxPanelSkillsListBox(void* panel);  // LB_SKILLS (rows)
void* SkillInfoBoxPanelMessageLabel(void* panel);   // LBL_MESSAGE (title)
void* SkillInfoBoxPanelOkButton(void* panel);       // BTN_OK

// CSWGuiScriptSelect (ScriptSelect.gui), the character sheet's
// combat-behaviour picker; KOTOR 1 only.
void* ScriptSelectPanelAiStateListBox(void* panel);  // LST_AIState
void* ScriptSelectPanelAcceptButton(void* panel);    // BTN_Accept ("Choose")
void* ScriptSelectPanelBackButton(void* panel);      // BTN_Back ("Cancel")

// CSWGuiWagerPopup (pazaakwager / pazaakwager_p), the pazaak bet popup.
void* WagerPopupPanelLessButton(void* panel);     // BTN_LESS (SpeedButton)
void* WagerPopupPanelMoreButton(void* panel);     // BTN_MORE (SpeedButton)
void* WagerPopupPanelMaximumLabel(void* panel);   // LBL_MAXIMUM (max + credits)

// CSWGuiPazaakStart (pazaaksetup / pazaaksetup_p), the side-deck builder.
void* PazaakStartPanelPlayButton(void* panel);    // BTN_ATEXT ("Play")
void* PazaakStartPanelClearButton(void* panel);   // BTN_CLEARCARDS (K2 only)

// CSWGuiInGameOptKeyMappings (OPTKeyMapping / OPTKeyMapping_p), the
// Keymapping screen. FilterButton takes the engine's own filter index
// (0 = MOVEMENT, 1 = GAME, 2 = MINIGAME), nullptr outside 0..2.
void* KeyMapPanelEventListBox(void* panel);       // LST_EventList
void* KeyMapPanelDefaultButton(void* panel);      // BTN_Default ("Standard")
void* KeyMapPanelAcceptButton(void* panel);       // BTN_Accept ("OK")
void* KeyMapPanelCancelButton(void* panel);       // BTN_Cancel ("Cancel")
void* KeyMapPanelFilterButton(void* panel, int filterIndex);

// CSWGuiUpgradeItemSelect (upgradeitems / upgradeitems_p), the workbench
// per-category item picker.
void* WorkbenchItemsPanelItemsListBox(void* panel);   // LB_ITEMS
void* WorkbenchItemsPanelUpgradeButton(void* panel);  // BTN_UPGRADEITEM
void* WorkbenchItemsPanelBackButton(void* panel);     // BTN_BACK

}  // namespace acc::menus::detail

// CSWGuiInGameItemEntry (LB_ITEMS row) — field6_0x394 bit-field after the
// embedded CSWGuiButton + item id + borders + text. OnEnterSlot tags the
// currently-equipped row via SetItem(id, /*param_2=*/1, 0); SetItem packs
// (param_2 & 1) into bit 1 (0x2) of field6_0x394 — the same flag that makes
// the engine append the selected-item suffix. So bit 0x2 set ⇔ "this row is
// the item currently equipped in the selected slot". Used by EquipPickerOnEnter
// to route Enter on the equipped row to an unequip (commit the row-0
// 0x7f000000 "empty" entry) instead of a no-op re-equip.
//
// KOTOR 2 from its own constructor, which builds the same members in the same
// order — button, then three borders, then a text — and then writes the two
// trailing fields: the 0x7f000000 "empty" sentinel into the item id at 0x1d0,
// and three bit-clears (0x1, 0x2, 0x4) into 0x3ac. Arithmetic agrees exactly:
// KOTOR 2's button is 0x1d0 and its border 0x78, so 0x1d0 + 4 + 3*0x78 + 0x70
// (text) = 0x3ac. Both the sentinel and the low three flag bits are the same
// values KOTOR 1 uses, which is what carries the bit index across.
const size_t kEquipItemEntryFlagsOffset  = acc::off::Pick(0x394, 0x3ac);
const uint32_t kEquipItemEntryEquippedBit = acc::off::Same(0x2);
