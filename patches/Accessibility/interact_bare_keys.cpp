// The bare action-bar keys 1-7: the announcement when the engine refused (an
// empty column or row); a real queue add is announced by the AddAction detour.

#include "interact_internal.h"

#include "combat_queue.h"
#include "engine_actionbar.h"
#include "engine_radial.h"
#include "log.h"
#include "speech.h"
#include "strings.h"
#include "unified_action_menu.h"

#include <cstdio>

namespace acc::interact {

namespace {

// The bare action-bar keys: the empty-column phrase, 1-based for the user.
void SpeakColumnEmpty(int index, char* msg, size_t cap) {
    std::snprintf(msg, cap, acc::strings::Get(acc::strings::Id::FmtActionBarColumnEmpty),
                  index + 1);
    acc::speech::Speak(msg, /*interrupt=*/true);
}

}  // namespace

// Bare keys 4-7 fire personal action-bar columns 0-3 in the engine itself (no
// hook on DoPersonalAction). A real queue add is announced by the AddAction
// detour (combat_queue's OnEngineActionAdded); this path only speaks the empty
// column, where the engine refused.
void AnnounceBarePersonalKey(int slot) {
    // -1 when input_pipeline consumed the press (a Shift+combo), so nothing
    // fired: stay silent rather than speak a phantom.
    const int preDepth = acc::combat::queue::GetPrePressDepth();
    if (preDepth < 0) {
        acclog::Write("ActionBar", "bare key slot=%d — no engine dispatch this "
            "press (preDepth=-1); skipping phantom announce", slot);
        return;
    }
    void* mi = acc::engine_actionbar::ResolveMainInterface();
    if (!mi) {
        acclog::Write("ActionBar", "bare key slot=%d — main_interface unresolved", slot);
        return;
    }
    char msg[128];
    const int nVar = acc::engine_actionbar::VariantCount(mi, slot);
    if (nVar <= 0) {
        SpeakColumnEmpty(slot, msg, sizeof(msg));
        acclog::Write("ActionBar", "bare key slot=%d variants=0 -> [%s]", slot, msg);
        return;
    }
    // The menu keeps this index in step with the engine's selected variant.
    int idx = acc::unified_menu::PersonalSelection(slot);
    if (idx < 0 || idx >= nVar) idx = 0;
    char label[128] = "";
    acc::engine_actionbar::ReadVariantLabel(mi, slot, idx, label, sizeof(label));
    if (label[0] == '\0') {
        SpeakColumnEmpty(slot, msg, sizeof(msg));
        acclog::Write("ActionBar", "bare key slot=%d variants=%d idx=%d "
            "label=empty -> [%s]", slot, nVar, idx, msg);
        return;
    }
    acclog::Write("ActionBar", "bare key slot=%d variants=%d idx=%d label=[%s] "
        "pre=%d — queued; announce via AddAction hook",
        slot, nVar, idx, label, preDepth);
}

// Bare keys 1-3: the target-action rows 0-2, the same way.
void AnnounceBareTargetKey(int row) {
    const int preDepth = acc::combat::queue::GetPrePressDepth();
    if (preDepth < 0) {
        acclog::Write("ActionBar", "bare target row=%d — no engine dispatch this "
            "press (preDepth=-1); skipping phantom announce", row);
        return;
    }
    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    if (!tam) {
        acclog::Write("ActionBar", "bare target row=%d — TAM unresolved", row);
        return;
    }
    const int count = acc::engine_radial::RowActionCount(tam, row);
    char label[128] = "";
    acc::engine_radial::ReadRowActionLabel(tam, row, label, sizeof(label));
    if (count <= 0 || label[0] == '\0') {
        char msg[128];
        SpeakColumnEmpty(row, msg, sizeof(msg));
        acclog::Write("ActionBar", "bare target row=%d count=%d label=[%s] -> [%s]",
            row, count, label, msg);
        return;
    }
    acclog::Write("ActionBar", "bare target row=%d label=[%s] "
        "pre=%d — queued; announce via AddAction hook", row, label, preDepth);
}

}  // namespace acc::interact
