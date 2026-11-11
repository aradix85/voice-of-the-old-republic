// Internal to the menus_listbox module (menus_listbox*.cpp); no other file includes
// it. The public interface is menus_listbox.h. Here: what the files share.

#pragma once

#include "menus_listbox.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_input.h"
#include "menus_internal.h"
#include "strings.h"

using namespace acc::engine;  // IdentifyPanel, PanelKind, kInput*, etc.

using acc::menus::detail::FindControlById;
using acc::menus::detail::IsSaveLoadPanel;
using acc::menus::detail::ReadSaveLoadEntryString;
using acc::menus::detail::DriveListBoxSelection;
using acc::menus::detail::DriveListBoxSelectionEngine;
using acc::menus::detail::ListBoxNavOp;
using acc::menus::detail::ListBoxNavResult;
using acc::menus::detail::QueueControlActivate;
using acc::menus::detail::ContainerPanelItemsListBox;
using acc::menus::detail::ContainerPanelOkButton;
using acc::menus::detail::ContainerPanelCancelButton;
using acc::menus::detail::ContainerPanelGiveItemsButton;
using acc::menus::detail::SkillInfoBoxPanelSkillsListBox;
using acc::menus::detail::SkillInfoBoxPanelMessageLabel;
using acc::menus::detail::SkillInfoBoxPanelOkButton;
using acc::menus::detail::ScriptSelectPanelAiStateListBox;
using acc::menus::detail::ScriptSelectPanelAcceptButton;
using acc::menus::detail::ScriptSelectPanelBackButton;
using acc::menus::detail::WorkbenchItemsPanelItemsListBox;
using acc::menus::detail::WorkbenchItemsPanelUpgradeButton;
using acc::menus::detail::WorkbenchItemsPanelBackButton;

// Every panel this file drives resolves its controls through the ctor-bound
// member resolvers in menus_internal.cpp (ContainerPanel*, SkillInfoBoxPanel*,
// ScriptSelectPanel*, WorkbenchItemsPanel*, the SaveLoad and picker sets) —
// no .gui ids are needed here any more. The historical ids live on only
// inside those resolvers, as GuiIdMismatch tripwires.

namespace acc::menus::listbox {

// ============================================================================
// Spec struct. One value per panel kind. All variation flows through the
// callback fields — the dispatcher itself is panel-agnostic.
// ============================================================================
struct ListBoxPanelSpec {
    // Diagnostic tag for log lines ("Container" / "SaveLoad" / "EquipPicker").
    const char* logTag;
    // True if this spec applies to `panel`. Panel-kind comparison or a
    // structural matcher (SaveLoad uses control-ID signature).
    bool (*matches)(void* panel);
    // True if the spec is currently armed. nullptr = always armed when
    // matches. Only the two pickers use it — they read their armed flag via
    // IsEquipPickerArmed / IsWorkbenchUpgradePickerArmed (state owned by
    // menus_listbox_picker.cpp).
    bool (*armed)();
    // Optional pre-dispatch hook to clean up stale state when the panel
    // pointer drifted (re-open). EquipPicker disarms if armed against an
    // older panel pointer. nullptr = no-op.
    void (*resetStale)(void* panel);
    // Locate the listbox we drive. Container / SaveLoad / EquipPicker
    // resolve their ctor-bound member (menus_internal.cpp); the panels
    // whose listbox the engine heap-allocates still use FindControlById.
    void* (*findListBox)(void* panel);
    // First valid selection index. 0 normally; 1 for EquipPicker (skips
    // the row 0 protoitem template).
    int minSel;
    // Format + speak the focused row. Called on every successful Up/Down
    // step (and on no-op clamps so the user gets feedback at boundaries).
    // Callback decides internally whether to actually speak — Container/
    // EquipPicker speak only on clamp (per-tick monitor handles normal
    // moves), SaveLoad speaks on every step (no monitor watches it).
    void (*announce)(void* lb, const ListBoxNavResult& r);
    // Optional per-row enrichment, fired AFTER `announce`. Used when the
    // spec needs to fetch supplementary speech text from an auxiliary
    // engine source — e.g. the SkillInfoBox spec calls
    // CSWGuiFeatsCharGen::OnEnterFeat(featId) on the underlying main panel
    // to repopulate its description_listbox, then reads + speaks it.
    //
    // Why split off `announce`: the row-name-and-position speech is a
    // common shape across all specs, but the "fetch from elsewhere and
    // speak extra context" path is intentionally a side-channel — it can
    // call into the engine, can fault, can early-return, and shouldn't
    // pollute the simpler announce path. Keeping the two callbacks
    // separate means a spec that just wants to speak a row title (Container)
    // doesn't have to think about enrichment, and vice versa.
    //
    // nullptr = no enrichment (most specs).
    void (*enrichRow)(void* panel, const ListBoxNavResult& r);
    // Optional extra text appended to the standard nav log line.
    // SaveLoad uses it to log planet/area; others leave it nullptr.
    void (*logExtra)(char* out, size_t outN, const ListBoxNavResult& r);
    // Handle Enter. Returns true if consumed. Called only when armed and
    // the key is Enter1/Enter2.
    bool (*onEnter)(void* panel);
    // Handle Esc. Returns true if consumed. nullptr = don't handle Esc.
    bool (*onEsc)(void* panel);
    // Optional title-speech override. Called from menus_focus.cpp's
    // AnnouncePanelTitle when a panel that this spec matches gets focus
    // for the first time. Returning a non-null string substitutes that
    // string for the generic title-walk speech; returning nullptr falls
    // back to the generic walk (first announceable label child).
    //
    // Used by the SkillInfoBox spec to replace the BioWare-leftover
    // placeholder ("Items Available to Place in Container and blah blah
    // blah") that the chargen flow doesn't override at runtime.
    //
    // nullptr = no override (most specs — the generic title walk works).
    const char* (*titleOverride)(void* panel);
    // Optional spoken phrase when the user navigates an empty listbox.
    // The dispatcher currently just logs `lb=... empty; nav ignored` and
    // stays silent; for panels where empty is a meaningful state worth
    // announcing (e.g. workbench items-picker when the player has no
    // upgradable weapons in the chosen category) this speaks the phrase
    // once per Up/Down press. Dedup happens on the speech channel so
    // mashing arrows doesn't spam the user.
    //
    // Count_ = no announcement (default).
    acc::strings::Id emptyStateId;
    // After dispatch, if the event was NOT consumed, should the caller
    // still return (skipping all subsequent handlers)?
    //
    //   * Container = true: arrow Left/Right and other unhandled keys on
    //     the loot panel should NOT fall through to chain navigation.
    //   * SaveLoad = false: unhandled keys should fall through so chain
    //     nav can still reach the Delete button.
    //   * EquipPicker = false: unhandled keys fall through to slot-zone
    //     navigation (the rest of OnHandleInputEvent).
    bool alwaysReturnFromHandler;
    // Optional dynamic first-row override. When non-null the dispatcher uses
    // its return value instead of the constant `minSel`. WorkbenchUpgrade uses
    // it because the picker's row 0 is a hidden remove entry on power slots but
    // a real choice on the colour slot. nullptr = use `minSel` (most specs).
    int (*minSelFn)(void* panel);
    // When true, Up/Down/Home/End drive the engine's own
    // CSWGuiListBox::SetSelectedControl (real row highlight + native multipage
    // scroll) via DriveListBoxSelectionEngine, instead of the raw selection_index
    // write of DriveListBoxSelection. Requires the cursor to be parked off the
    // list while armed — the picker arm path warps it to BTN_BACK — so the
    // engine's per-frame hover-select (HandleMouseMove → SetSelectedControl on
    // the row under the cursor) can't revert the selection. The workbench
    // upgrade and equipment pickers use it; everything else keeps the raw write.
    // false = raw write (default).
    bool useEngineSelect;
    // Optional liveness gate for nav keys. When non-null and returning
    // false, Up/Down/Home/End neither drive the listbox nor leak to the
    // engine; the dispatcher speaks emptyStateId instead (same dedup
    // channel as the empty-list case). Dialog specs use it to refuse
    // navigation while the engine's reply store for the current node is
    // still empty — i.e. the NPC line is playing and the replies are not
    // up yet. Without the gate the rows navigated were the PREVIOUS
    // node's stale row objects (the panel and listbox live for the whole
    // conversation): the selection moved and clicked but no reply text
    // existed anywhere to speak, which read as "dialogue options are
    // broken" (a user report).
    // nullptr = always live (every non-dialog spec).
    bool (*navLive)(void* panel);
    // Optional Left/Right handler, dir -1 / +1, for panels that stack several
    // lists behind view buttons (KOTOR 2 messages). Returns true if consumed;
    // false lets the key fall through as before. nullptr = not handled.
    bool (*onLeftRight)(void* panel, int dir);
};
extern const ListBoxPanelSpec kContainerSpec;
extern const ListBoxPanelSpec kSaveLoadSpec;
extern const ListBoxPanelSpec kEquipPickerSpec;
extern const ListBoxPanelSpec kSkillInfoBoxSpec;
extern const ListBoxPanelSpec kInGameMessagesSpec;
extern const ListBoxPanelSpec kDialogCinematicSpec;
extern const ListBoxPanelSpec kDialogCinematicCopySpec;
extern const ListBoxPanelSpec kDialogComputerSpec;
extern const ListBoxPanelSpec kDialogComputerCameraSpec;
extern const ListBoxPanelSpec kWorkbenchItemsSpec;
extern const ListBoxPanelSpec kWorkbenchUpgradeSpec;
extern const ListBoxPanelSpec kExamineSpec;
extern const ListBoxPanelSpec kScriptSelectSpec;

// Helpers shared across the files, by the file that defines them.

// menus_listbox.cpp
void SpeakRowAndPosition(const ListBoxNavResult& r);
bool SetListBoxSelectedControl(void* listbox, int index);

// menus_listbox_container.cpp
void MonitorContainerSelection();
void PollContainerGiveModeKey();

// menus_listbox_saveload.cpp
void MonitorK2SaveLoadSelection();
void AnnounceK2SaveLoadInfo();
void MonitorK2SaveLoadCharacterSwap();

}  // namespace acc::menus::listbox
