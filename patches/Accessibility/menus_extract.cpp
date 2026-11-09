// control text extraction (the announce ladder).
//
// FromControl is the ladder: it does no extraction of its own, it runs the
// Try* steps in order (one file per kind of control, menus_extract_*.cpp)
// and applies the three announce suffixes. Each step's rationale lives at
// its definition, under its step number. Helpers shared with chain
// navigation (IsChainNavigable, IsClassSelectionIcon, ClassLabelCache*,
// GetControlCenter) and the focused-panel pointer (g_currentPanel) come
// from menus_internal.h.

#include "menus_extract_internal.h"

#include "engine_manager.h"
#include "engine_panels.h"
#include "menus_listbox.h"      // IsEquipPickerArmed / IsWorkbenchUpgradePickerArmed
#include "menus_modsettings.h"
#include "strings.h"

namespace acc::menus::extract {

namespace {

// Cycle-category cache. Cycle widgets render as `[◀] value [▶]`; on each
// activation the engine rewrites the middle button's CExoString to the
// new value, losing the category name. We capture the (control →
// category) mapping at panel-walk time, BEFORE any activation runs, so
// FromControl can prepend the category in announce output.
//
// Populated externally via the public CaptureCycleCategory setter
// (called from OnSetActiveControl in menus.cpp on first focus into a
// panel). Cleared on the same path via ResetCycleCategoryCache.
// Invalidated implicitly on every RebindChain because the panel-walk
// path always resets-then-recaptures.
struct CycleCategoryEntry {
    void* control;
    char  category[128];
};
constexpr int kMaxCycleCategoryEntries = 16;
CycleCategoryEntry s_cycleCategories[kMaxCycleCategoryEntries];
int s_cycleCategoryCount = 0;

const char* LookupCycleCategory(void* control) {
    for (int i = 0; i < s_cycleCategoryCount; ++i) {
        if (s_cycleCategories[i].control == control) {
            return s_cycleCategories[i].category;
        }
    }
    return nullptr;
}

// Cycle value-display prefix. Cycle widgets render as `[◀] value [▶]`
// and the engine rewrites the middle button's CExoString to the new
// value on each activate, losing the category name. We capture the
// category at panel-walk time (in OnSetActiveControl, before any
// activation has run); here we just look it up. Skipped for toggles
// (whose own text already reads as "{label}, {state}") and for
// non-cycle buttons (LookupCycleCategory returns null when control
// isn't in the cycle map). Redundancy guard: if the captured
// "category" is byte-identical to the current rendered value we
// suppress the prefix — that's the failure mode where capture caught
// the value rather than the category (timing-dependent; see
// OnSetActiveControl), and "Normal, Normal" is worse than just "Normal".
void ApplyCycleCategoryPrefix(void* control, char* outBuf, size_t bufSize) {
    const char* category = LookupCycleCategory(control);
    if (category && strcmp(category, outBuf) != 0) {
        char value[256];
        strncpy_s(value, outBuf, _TRUNCATE);
        snprintf(outBuf, bufSize, "%s, %s", category, value);
    }
}

// Append element-state suffix for toggles. Detected via the same downcast
// we'd use for text extraction, so works regardless of which path
// returned the label (most toggles are caught by AsButton at step 2).
void AppendToggleSuffix(void* control, char* outBuf, size_t bufSize) {
    bool on = ReadToggleState(control);
    size_t len = strnlen(outBuf, bufSize);
    const char* suffix = acc::strings::Get(
        on ? acc::strings::Id::ToggleOn : acc::strings::Id::ToggleOff);
    size_t suffixLen = strlen(suffix);
    if (len + suffixLen + 1 <= bufSize) {
        memcpy(outBuf + len, suffix, suffixLen + 1);
    }
}

// Append "nicht verfügbar" / "unavailable" suffix when the focused
// button is engine-disabled. CSWGuiControl.bit_flags (+0x44 on KOTOR 1,
// +0x48 on KOTOR 2 — use kControlBitFlagsOffset, never the literal; a
// hardcoded 0x44 here read four bytes low on KOTOR 2 and made EVERY
// navigable control on every panel announce as unavailable) layout:
//
//   bit 0 (0x1) — selection state (CSWGuiControl::SetActive)
//   bit 1 (0x2) — interactive / accepts click (general gate)
//   bit 2 (0x4) — visible           (CSWGuiPanel::SetVisible)
//   bit 3 (0x8) — enabled flag      (CSWGuiControl::SetEnabled @0x004176a0)
//
// For most panels bit 1 is the right "does a click dispatch" signal.
// The InGameLevelUp wizard is the exception: it enforces sequential
// leveling, enabling exactly ONE category at a time via SetEnabled
// (bit 3). Its non-current categories keep bit 1 set but bit 3 clear,
// so on that panel we key the suffix on bit 3 — otherwise only the
// no-points categories (bit 1 clear) would read as unavailable and the
// not-yet-your-turn ones (e.g. Kräfte before Fähigkeiten) would sound
// actionable. menus_chain blocks their activation on the same bit 3, so
// the announcement and the gate agree. Verified in
// patch-20260608-125730/125909: current step 0x..8f, others 0x..06/0x..04;
// Annehmen gains bit 3 only once every step is done.
//
// Skipped for toggles — their ", ein" / ", aus" suffix already
// disambiguates state. Gated on IsChainNavigable so labels don't get
// the suffix.
void AppendDisabledSuffix(void* control, void* ownerPanel,
                          char* outBuf, size_t bufSize) {
    // While a row picker is armed, the engine clears the interactive bit on
    // the buttons of the panel underneath it — the equip slots, the workbench
    // slots — because the picker owns input until it commits or cancels. That
    // is "not right now", not "not available to you", and the suffix says the
    // latter. Every item swap therefore re-announced the slot as unavailable
    // the instant its text changed: "Body, Echani Fiber Armor, unavailable",
    // sixteen distinct lines in the beta log and present as far back as the
    // 0.5.7 logs.
    //
    // This suppression is exact rather than approximate, because both IsArmed
    // queries now READ the engine's own picker-open bit (see
    // menus_listbox_picker.cpp). On the equip screen the very call that raises
    // that bit, ShowDescription(1), is what cleared these interactive bits, and
    // the call that clears it puts them back — so "armed" and "the slots are
    // disabled by the picker" are the same fact, not two facts that have to be
    // kept in step. While they WERE two facts, this early-out could only paper
    // over the mismatch: an Escape cleared our flag and never told the engine,
    // so the suffix came back on for slots the engine had left disabled for
    // good, and every slot read "unavailable" until the screen was reopened.
    //
    // Not gated on ownerPanel: an armed picker owns its panel exclusively (the
    // bit lives on that panel and dies with it), and several announce paths
    // reach here with no owner resolved.
    if (acc::menus::listbox::IsEquipPickerArmed() ||
        acc::menus::listbox::IsWorkbenchUpgradePickerArmed()) {
        return;
    }

    uint32_t bitFlags = *reinterpret_cast<uint32_t*>(
        reinterpret_cast<unsigned char*>(control) + kControlBitFlagsOffset);
    uint32_t disabledMask = 0x2;
    if (ownerPanel) {
        PanelKind k = IdentifyPanel(ownerPanel);
        if (k == PanelKind::InGameLevelUp || k == PanelKind::CharGen) {
            disabledMask = 0x8;
        }
    }
    if ((bitFlags & disabledMask) == 0) {
        const char* suffix = acc::strings::Get(
            acc::strings::Id::DisabledSuffix);
        size_t len = strnlen(outBuf, bufSize);
        size_t suffixLen = strlen(suffix);
        if (suffixLen > 0 && len + suffixLen + 1 <= bufSize) {
            memcpy(outBuf + len, suffix, suffixLen + 1);
        }
    }
}

}  // namespace

// Hand `text` back to the caller, TRUNCATING when it does not fit. Returns
// false only when there is genuinely nothing to say.
//
// A plain `if (len + 1 <= bufSize) memcpy(...)` per step would let a
// caller's buffer size decide whether a control HAS a name,
// not merely how much of it you hear. The two probe call sites pass 64 bytes
// and only test the return value (FindAdjacentArrow, SquashCycleFlankers), so
// any control whose label ran past 63 characters read as text-less to them
// and could be squashed out of the chain entirely. Truncation is the right
// failure: the rest of the ladder (every snprintf path) already truncates.
bool EmitText(char* outBuf, size_t bufSize, const char* text) {
    if (!outBuf || bufSize < 2 || !text || text[0] == '\0') return false;
    strncpy_s(outBuf, bufSize, text, _TRUNCATE);
    return outBuf[0] != '\0';
}

// ---------------------------------------------------------------------
// Announce-ladder steps.
//
// FromControl is a first-match-wins ladder: each step below inspects the
// control and either fills outBuf and returns its source tag, or returns
// nullptr to let the next step try. The steps keep the numbering they had
// when they were inline sections of FromControl, so a log line reading
// src=perkind-equip-slot still points at the step named for it here.
//
// One step deviates from that contract and says so at its own
// definition: TryPartyPortrait (step 7b) reports whether it claimed the
// control even when it produced no text.
// ---------------------------------------------------------------------

// Resolve the panel that owns `control`. Caller-passed owner wins
// (RebindChain, WalkChildren, BuildContentFingerprint, SetActiveControl
// all know the panel). Otherwise scan panels[] — covers callers that don't
// know the panel (AnnounceControl from chain-step, listbox-row helpers,
// FindCloseButton from the input hook). g_currentPanel is the last-resort
// fallback for early-attach windows where the manager isn't resolvable yet.
//
// The IsPanelInManager filter drops stale/wild owner pointers: the
// resolution chain can surface a freed g_currentPanel right after a
// commit-style button (Annehmen on InGameLevelUp) synchronously destroys
// its panel inside our FireActivate dispatch. IsPanelInManager is
// deref-free, so it's safe even when the candidate is wild. Every consumer
// below (IdentifyPanel, IsStatRowAnchor, IsClassSelectionIcon,
// IsPanelKindInGameMenu) dereferences the panel vtable and would AV
// otherwise.
void* ResolveOwnerPanel(void* control, void* ownerPanel) {
    void* owner = ownerPanel;
    if (!owner) owner = FindOwningPanel(control);
    if (!owner) owner = g_currentPanel;
    if (owner && !acc::engine::IsPanelInManager(owner)) owner = nullptr;
    return owner;
}

// True when `control` sits directly in panel.controls[]. Distinguishes a
// panel-direct widget (screen-space extent, comparable with its siblings)
// from a listbox row (listbox-local content extent, comparable with
// nothing outside its own list). Same 256 cap the chain walk and
// FindSiblingLabel use, so the three agree on what "on this panel" means.
bool IsPanelDirectChild(void* panel, void* control) {
    if (!panel || !control) return false;
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
        if (!list || !list->data) return false;
        int n = list->size > 256 ? 256 : list->size;
        for (int i = 0; i < n; ++i) {
            if (list->data[i] == control) return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}

void ResetCycleCategoryCache() {
    s_cycleCategoryCount = 0;
}

void CaptureCycleCategory(void* control, const char* category) {
    if (!control || !category) return;
    // Upsert: if `control` is already in the cache, replace its category
    // text. This lets a panel-specific override (e.g. chargen Attribute's
    // ability_button → ability_label binding in menus_chargen_attr.cpp)
    // replace whatever the generic capture loop registered earlier this
    // panel-walk pass — without an upsert, `LookupCycleCategory` returns
    // the first hit and the override silently loses.
    for (int i = 0; i < s_cycleCategoryCount; ++i) {
        if (s_cycleCategories[i].control == control) {
            strncpy_s(s_cycleCategories[i].category, category, _TRUNCATE);
            return;
        }
    }
    if (s_cycleCategoryCount >= kMaxCycleCategoryEntries) return;
    s_cycleCategories[s_cycleCategoryCount].control = control;
    strncpy_s(s_cycleCategories[s_cycleCategoryCount].category,
              category, _TRUNCATE);
    ++s_cycleCategoryCount;
}

const char* FromControl(void* control,
                        char* outBuf, size_t bufSize,
                        void* ownerPanel) {
    if (!control || bufSize < 2) return nullptr;

    // -1. Virtual-entry short-circuit. The mod-settings root chain entry
    //     uses a static sentinel pointer as `control`; it is NOT an
    //     engine-allocated CSWGuiControl and dereferencing it (vtable
    //     reads, struct-offset loads) would AV. Match by pointer equality
    //     against the registered sentinel and emit the localised label.
    //     Runs ahead of every other section so no downstream reader
    //     touches the sentinel.
    if (acc::menus::modsettings::IsRootAnchor(control)) {
        if (acc::menus::modsettings::ExtractRootLabel(outBuf, bufSize) &&
            outBuf[0] != '\0') {
            return "virtual-modsettings-root";
        }
        outBuf[0] = '\0';
        return nullptr;
    }

    const char* source = nullptr;

    // Resolved up front, not just before the per-kind fallbacks: TrySlider
    // needs the owning panel too (its category label is a spatial sibling).
    void* owner = ResolveOwnerPanel(control, ownerPanel);

    if (!source) source = TryVirtualRowAnchor(control, ownerPanel, outBuf, bufSize);
    if (!source) source = TryTooltip(control, outBuf, bufSize);
    if (!source) source = TryButton(control, outBuf, bufSize);
    if (!source) source = TryButtonToggle(control, outBuf, bufSize);
    if (!source) source = TryLabel(control, outBuf, bufSize);
    if (!source) source = TryLabelHilight(control, outBuf, bufSize);
    if (!source) source = TrySlider(control, owner, outBuf, bufSize);
    if (!source) source = TryEditbox(control, outBuf, bufSize);
    if (!source) source = TrySingleRowListBox(control, outBuf, bufSize);
    if (!source) {
        bool isPartyPortrait = false;
        source = TryPartyPortrait(control, outBuf, bufSize, isPartyPortrait);
        // Skip the speculative reader for this vtable — it never resolves
        // anything useful for portraits.
        if (!source && isPartyPortrait) return nullptr;
    }
    if (!source) source = TrySpeculativeVtableRead(control, outBuf, bufSize);

    // The per-kind fallbacks below all need the owning panel too.
    if (!source) source = TryInGameMenuIcon(control, owner, outBuf, bufSize);
    if (!source) source = TryEquipSlot(control, owner, outBuf, bufSize);
    if (!source) source = TryInGameMapArrow(control, owner, outBuf, bufSize);
    if (!source) source = TryWorkbenchSlot(control, owner, outBuf, bufSize);
    if (!source) source = TryWorkbenchCreateItemsButton(control, owner, outBuf, bufSize);
    if (!source) source = TryClassSelectionIcon(control, owner, outBuf, bufSize);
    if (!source) source = TryPortraitCharGenArrow(control, owner, outBuf, bufSize);
    if (!source) source = TryItemEntryRow(control, outBuf, bufSize);
    if (!source) source = TrySiblingLabel(control, owner, outBuf, bufSize);

    if (source && !IsToggle(control)) {
        ApplyCycleCategoryPrefix(control, outBuf, bufSize);
    }
    if (source && IsToggle(control)) {
        AppendToggleSuffix(control, outBuf, bufSize);
    }
    if (source && !IsToggle(control) && IsChainNavigable(control)) {
        AppendDisabledSuffix(control, ownerPanel, outBuf, bufSize);
    }

    return source;
}

}  // namespace acc::menus::extract
