// listbox-driven panel input dispatcher.
//
// See menus_listbox.h for the motivation. This file holds:
//
//   * The ListBoxPanelSpec struct (private) — one entry per panel kind.
//   * All thirteen spec entries. Each carries ~5-6 small static callbacks
//     that capture its quirks (announce format, button IDs, custom Enter
//     dispatch, etc.).
//   * The dispatcher TryHandleInput that walks the table.
//   * The container monitor + the container give-mode key poll.
//
// The two armed pickers (equipment, workbench upgrade) keep their SPECS here
// with the other eleven, but their mutable state and per-tick monitors live
// in menus_listbox_picker.cpp — see that file's header. The spec callbacks
// reach the state through the accessors in menus_listbox.h.

#include "menus_listbox_internal.h"

#include "engine_offsets.h"
#include "log.h"
#include "menus.h"           // SpeakIfChanged (empty-state dedup)
#include "menus_extract.h"
#include "speech.h"

namespace acc::menus::listbox {

namespace {

// The in-game Faehigkeiten screen (InGameAbilities) is NOT a ListBoxPanelSpec.
// Its ability_listbox rows are CSWGuiSkillFlow controls and the tabs need
// per-tab routing (Skills is keyboard-driven by us via OnEnterSkill; Feats and
// Powers are engine-native charts). It has a dedicated handler in
// menus_abilities.cpp (acc::menus::abilities::HandleInput), invoked from
// menus.cpp before this dispatcher.
// Spec table. Probe order matters: SaveLoad's structural matcher
// (FindControlById signature check) is a superset that could in principle
// match other panels with the same control IDs — Container and EquipPicker
// have distinct PanelKind values, so they probe first by identity. In
// practice the matchers are disjoint; ordering here is just defensive.
constexpr const ListBoxPanelSpec* kSpecs[] = {
    &kContainerSpec,
    &kSaveLoadSpec,
    &kEquipPickerSpec,
    &kSkillInfoBoxSpec,
    // Combat-system plan, Phase 1C/1D — read-only review screens.
    &kInGameMessagesSpec,
    &kDialogCinematicSpec,
    &kDialogCinematicCopySpec,
    &kDialogComputerSpec,
    &kDialogComputerCameraSpec,
    // Workbench panels (Phase: workbench accessibility).
    &kWorkbenchItemsSpec,
    &kWorkbenchUpgradeSpec,
    // Combat-system plan, Phase 2C — Ö Examine engine panel.
    &kExamineSpec,
    // Character sheet "Kurzbefehle" combat-behaviour picker.
    &kScriptSelectSpec,
};
constexpr int kNumSpecs = static_cast<int>(sizeof(kSpecs) / sizeof(kSpecs[0]));

// ============================================================================
// Dispatcher — generic over the spec table. Mirrors the structure of the
// original three inline blocks in menus.cpp's OnHandleInputEvent.
// ============================================================================
// How long a nav refusal stays deduped. Long enough that holding the key down
// (auto-repeat is far faster) yields one line rather than a machine-gun, short
// enough that a deliberate second press always gets an answer. A refusal is a
// reply to a question the user just asked; answering only the first time reads
// as the mod having gone deaf.
constexpr unsigned kRefusalRepeatMs = 1500;

bool DispatchKeyDownEdge(const ListBoxPanelSpec& spec, void* panel,
                         int param_1)
{
    // Up / Down / Home / End: drive the listbox cursor + announce +
    // optional enrichment. Home / End are absolute jumps to the first
    // (minSel) / last (rowCount-1) row; Up / Down are Â±1 steps. The
    // announce + enrichment + log paths are shape-identical across all
    // four — only the ListBoxNavOp selection and the direction tag differ.
    ListBoxNavOp op;
    const char* dirTag = nullptr;
    if      (param_1 == kInputNavUp)   { op = ListBoxNavOp::StepUp;    dirTag = "Up";   }
    else if (param_1 == kInputNavDown) { op = ListBoxNavOp::StepDown;  dirTag = "Down"; }
    else if (param_1 == kInputHome)    { op = ListBoxNavOp::JumpFirst; dirTag = "Home"; }
    else if (param_1 == kInputEnd)     { op = ListBoxNavOp::JumpLast;  dirTag = "End";  }

    if (dirTag) {
        void* lb = spec.findListBox(panel);
        // Liveness gate (dialog specs): the rows currently in the listbox
        // may be the previous node's leftovers with no readable text behind
        // them. Refuse to move the cursor over them — consume the key (so
        // the engine can't act on it either) and report the state instead of
        // clicking through unreadable rows.
        //
        // The phrase is deliberately neutral ("no reply options") rather than
        // "not yet available": this same gate covers a cinematic that never
        // offers a choice at all, so a wording that promises replies are
        // coming would be wrong there. It also carries no hint about which
        // key to press — the right key differs per situation, and a wrong
        // hint is worse than none.
        if (lb && spec.navLive && !spec.navLive(panel)) {
            acclog::Write(spec.logTag, "%s lb=%p gated: reply set not live; "
                          "nav refused", dirTag, lb);
            if (spec.emptyStateId != acc::strings::Id::Count_) {
                const char* phrase = acc::strings::Get(spec.emptyStateId);
                if (phrase && phrase[0] != '\0') {
                    acc::menus::SpeakIfChanged(/*channel=*/1, phrase,
                                               kRefusalRepeatMs);
                }
            }
            return true;
        }
        int effMinSel = spec.minSelFn ? spec.minSelFn(panel) : spec.minSel;
        ListBoxNavResult r;
        bool moved = lb && (spec.useEngineSelect
            ? DriveListBoxSelectionEngine(lb, op,
                                          static_cast<short>(effMinSel), r)
            : DriveListBoxSelection(lb, op,
                                    static_cast<short>(effMinSel), r));
        if (moved) {
            // Specs with no announce callback (dialogue replies) drive the
            // cursor + consume the key here but defer speech to a poll monitor.
            if (spec.announce) spec.announce(lb, r);
            if (spec.enrichRow) spec.enrichRow(panel, r);
            char extra[128] = {0};
            if (spec.logExtra) spec.logExtra(extra, sizeof(extra), r);
            acclog::Write(spec.logTag, "%s lb=%p sel=%d->%d (rows=%d)%s",
                          dirTag, lb, r.oldSel, r.newSel, r.rowCount, extra);
        } else if (lb) {
            acclog::Write(spec.logTag, "%s lb=%p empty; nav ignored",
                          dirTag, lb);
            // Announce the empty state if the spec provided one, re-speaking
            // it when the user asks again after a pause — same reasoning as
            // the liveness refusal above.
            if (spec.emptyStateId != acc::strings::Id::Count_) {
                const char* phrase = acc::strings::Get(spec.emptyStateId);
                if (phrase && phrase[0] != '\0') {
                    acc::menus::SpeakIfChanged(/*channel=*/1, phrase,
                                               kRefusalRepeatMs);
                }
            }
        }
        return true;  // never let nav keys leak to the engine here
    }

    // Left / Right: the spec's view switch, if it has one.
    if ((param_1 == kInputNavLeft || param_1 == kInputNavRight) &&
        spec.onLeftRight) {
        return spec.onLeftRight(panel, param_1 == kInputNavLeft ? -1 : 1);
    }

    // Enter: dispatch via the spec's onEnter callback.
    if ((param_1 == kInputEnter1 || param_1 == kInputEnter2) && spec.onEnter) {
        return spec.onEnter(panel);
    }

    // Esc: dispatch via the spec's onEsc callback.
    if ((param_1 == kInputEsc1 || param_1 == kInputEsc2) && spec.onEsc) {
        return spec.onEsc(panel);
    }

    // Other keys (Left/Right, etc.): not consumed.
    return false;
}

void LogStandard(int n, void* thisPtr, int param_1, int param_2,
                 bool consumed)
{
    int translated = acc::engine::ManagerTranslateCode(param_1);
    const char* tag = consumed ? " CONSUMED" : "";
    if (translated != param_1) {
        acclog::Write("Menus.Input", "#%d this=%p key=logical(%d) -> %s(%d) val=%d%s",
                      n, thisPtr, param_1,
                      acc::engine::InputIndexName(translated), translated,
                      param_2, tag);
    } else {
        acclog::Write("Menus.Input", "#%d this=%p key=%s(%d) val=%d%s",
                      n, thisPtr, acc::engine::InputIndexName(param_1),
                      param_1, param_2, tag);
    }
}

}  // namespace

// The plain "<row text>, N von M" announce. Three specs want exactly this
// and had it written out three times, differing only in buffer sizes.
// Sized to the largest of the three, so nothing that used to fit still
// truncates.
void SpeakRowAndPosition(const ListBoxNavResult& r) {
    if (!r.row) return;
    char rowText[512];
    if (!acc::menus::extract::FromControl(r.row, rowText, sizeof(rowText))) {
        return;
    }
    char msg[640];
    snprintf(msg, sizeof(msg),
             acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
             rowText, r.newSel + 1, r.rowCount);
    acc::speech::Speak(msg, /*interrupt=*/false);
}

bool TryHandleInput(int n, void* thisPtr, void* activePanel,
                    int param_1, int param_2, int& outRv)
{
    if (!activePanel) return false;

    for (int i = 0; i < kNumSpecs; ++i) {
        const ListBoxPanelSpec& spec = *kSpecs[i];
        if (!spec.matches(activePanel)) continue;

        // Match. Run the optional stale-reset hook regardless of armed
        // state (EquipPicker disarms here when the panel pointer drifts).
        if (spec.resetStale) spec.resetStale(activePanel);

        // If the spec is gated by an armed-check and isn't armed, fall
        // through so the rest of OnHandleInputEvent can run (chain nav,
        // slot-zone nav). Don't log here — the caller's outer log path
        // takes over.
        if (spec.armed && !spec.armed()) return false;

        bool consumed = false;
        if (param_2 != 0) {
            consumed = DispatchKeyDownEdge(spec, activePanel, param_1);
        }

        if (consumed) {
            LogStandard(n, thisPtr, param_1, param_2, /*consumed=*/true);
            outRv = 1;
            return true;
        }

        if (spec.alwaysReturnFromHandler) {
            // Container behaviour: even on unhandled keys, return 0
            // without falling through to chain nav. The original code
            // logged the standard line + returned (consumed ? 1 : 0).
            LogStandard(n, thisPtr, param_1, param_2, /*consumed=*/false);
            outRv = 0;
            return true;
        }

        // Not consumed and the spec wants fall-through (SaveLoad,
        // EquipPicker armed-but-unhandled). Caller's outer handler runs
        // and takes responsibility for logging.
        return false;
    }
    return false;
}

// Index of `control` among the listbox's row children, or -1. SEH-guarded:
// the chain can outlive a control by a tick.
int FindListBoxRowIndex(void* listbox, void* control) {
    if (!listbox || !control) return -1;
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(listbox) + kListBoxControlsOffset);
        if (!list->data || list->size <= 0) return -1;
        int n = list->size > 512 ? 512 : list->size;
        for (int i = 0; i < n; ++i) {
            if (list->data[i] == control) return i;
        }
        return -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// The engine's own selection commit — real row highlight, native scroll, and
// the onSelectionChanged work the preview labels depend on. playSound=0: the
// chain step has already spoken the row, and the engine's click blip on top of
// that reads as a stutter.
bool SetListBoxSelectedControl(void* listbox, int index) {
    if (!listbox || index < 0) return false;
    __try {
        auto setSel = reinterpret_cast<PFN_CSWGuiListBoxSetSelectedControl>(
            kAddrCSWGuiListBoxSetSelectedControl);
        setSel(listbox, index, /*playSound=*/0);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void TickListboxMonitors() {
    MonitorContainerSelection();
    TickPickerMonitors();  // menus_listbox_picker.cpp — equip + workbench upgrade
    PollContainerGiveModeKey();
    MonitorK2SaveLoadCharacterSwap();  // rebind a swap-staled chain before the two below read it
    AnnounceK2SaveLoadInfo();     // consumes last tick's sync
    MonitorK2SaveLoadSelection();
}

const char* GetTitleOverride(void* panel) {
    if (!panel) return nullptr;
    for (int i = 0; i < kNumSpecs; ++i) {
        const ListBoxPanelSpec& spec = *kSpecs[i];
        if (!spec.matches(panel)) continue;
        if (!spec.titleOverride) return nullptr;
        return spec.titleOverride(panel);
    }
    return nullptr;
}

}  // namespace acc::menus::listbox
