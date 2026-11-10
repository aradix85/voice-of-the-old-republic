// The menu chain: one flat, top-to-bottom list of a panel's navigable controls that the arrow keys walk. Here: its state, the checks that the bound panel is still the same live panel, and RebindChain, which builds the list.

#include "menus_chain_internal.h"

#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_skills.h"
#include "menus_charsheet.h"
#include "menus_credits.h"
#include "menus_equipstats.h"
#include "menus_extract.h"
#include "menus_focus.h"     // WalkAndCaptureOnFirstSight — see RebindChain
#include "menus_modsettings.h"
#include "menus_crafting.h"  // IsHiddenCraftingListBox — K2 crafting view filter
#include "speech.h"
#include "strings.h"

namespace acc::menus::chain {

namespace {

// IsModalTextPanel was a file-static helper inside menus.cpp; only
// RebindChain calls it, so it lives here now (anonymous namespace).

bool IsModalTextPanel(PanelKind k) {
    switch (k) {
    case PanelKind::MessageBoxModal:
    case PanelKind::TutorialBox:
    case PanelKind::AreaTransition:
        return true;
    // StatusSummary deliberately omitted: its body is a label cluster, not a
    // single-row listbox, so this listbox path never applies. RebindChain
    // exposes its visible rows via the dedicated per-kind block below.
    default:
        return false;
    }
}

// Per-rebind chain dump. One line for the rebind plus one per entry;
// this is the trace we read back when a control goes missing from
// arrow navigation or announces the wrong text.
void LogChainContents(void* panel, void* active) {
acclog::Write("Menus.Chain", "rebind panel=%p count=%d index=%d active=%p "
              "equipSlotOffsetY=%d classIconOffsetX=%d",
              panel, g_chainCount, g_chainIndex, active,
              g_equipSlotClickOffsetY, g_classIconClickOffsetX);
for (int i = 0; i < g_chainCount; ++i) {
    char text[256];
    const char* src = acc::menus::extract::FromControl(g_chain[i].control,
                                              text, sizeof(text),
                                              panel);
    // Per-game offsets, not literals: on KOTOR 2 both fields sit +4 and the
    // hardcoded KOTOR 1 values printed a neighbouring dword, which made
    // every K2 chain dump look like garbage flags (bit_flags=0x10,
    // is_active=434) and sent one investigation chasing a phantom.
    unsigned int isActive =
        *reinterpret_cast<unsigned int*>(
            reinterpret_cast<unsigned char*>(g_chain[i].control) +
            kControlIsActiveOffset);
    unsigned int bitFlags =
        *reinterpret_cast<unsigned int*>(
            reinterpret_cast<unsigned char*>(g_chain[i].control) +
            kControlBitFlagsOffset);
    // sortCy is only printed when it differs from cy — i.e. for virtual rows
    // and listbox blocks — so the common case stays as terse as before.
    char sortNote[32];
    sortNote[0] = '\0';
    if (g_chain[i].sortCy != g_chain[i].cy) {
        snprintf(sortNote, sizeof(sortNote), " sort=%d", g_chain[i].sortCy);
    }
    acclog::Write("Menus.Chain", "  [%d] %p (%d,%d)%s%s %s text=\"%s\" is_active=%u bit_flags=0x%x",
                  i, g_chain[i].control, g_chain[i].cx, g_chain[i].cy,
                  sortNote,
                  g_chain[i].textOnly ? " text-only" : "",
                  src ? src : "?", src ? text : "", isActive, bitFlags);
}
}

}  // namespace

// Panels[]-membership check, shared by Validate{Tabbed,Chain}Panel and the
// DetectTabsCluster precondition. The save-popup teardown frees the SaveLoad
// panel synchronously when the user commits a save; the next
// OnListBoxSetActiveControl fires on the underlying in-game-menu's tooltip
// listbox while g_currentPanel still points at the freed SaveLoad
// allocation. By then the heap allocator has typically reused that block for
// combat-log strings, so panel+0x20/+0x24 (CExoArrayList data+size) come
// back as ASCII text — the data[0] deref then takes an AV
// (crash analysed 2026-05-29, dump swkotor.exe(1).31228.dmp).
bool IsPanelLive(void* panel) {
    if (!panel) return false;
    constexpr int kCap = 32;
    void* panels[kCap];
    int n = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panels, kCap);
    for (int i = 0; i < n; ++i) {
        if (panels[i] == panel) return true;
    }
    return false;
}

// ============================================================================
// Reads.
// ============================================================================

void* ReadPanelActiveControl(void* panel) {
    if (!panel) return nullptr;
    return *reinterpret_cast<void**>(
        reinterpret_cast<unsigned char*>(panel) + kPanelActiveControlOffset);
}

int FindChainEntry(void* control) {
    if (!control) return -1;
    for (int i = 0; i < g_chainCount; ++i) {
        if (g_chain[i].control == control) return i;
    }
    return -1;
}

// Speak guidance when the user activates an InGameLevelUp category that the
// engine has not currently enabled. The wizard enforces sequential leveling:
// exactly one category button carries bit_flags bit 3 (0x8 =
// CSWGuiControl::SetEnabled) at a time. Find it and name it so a blind user
// knows which step to do next; fall back to a generic "not your turn" line if
// none resolves. bit 3 verified against the current step in
// patch-20260608-125730/125909 (current 0x..8f, others 0x..06/0x..04).
void SpeakLevelUpDoStepFirst() {
    for (int i = 0; i < g_chainCount; ++i) {
        void* c = g_chain[i].control;
        if (!c) continue;
        uint32_t bf = 0;
        __try {
            bf = *reinterpret_cast<uint32_t*>(
                reinterpret_cast<unsigned char*>(c) + kControlBitFlagsOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            continue;
        }
        if ((bf & 0x8) == 0) continue;
        char name[128];
        const char* src = acc::menus::extract::FromControl(
            c, name, sizeof(name), g_chainPanel);
        if (src && name[0]) {
            char msg[192];
            snprintf(msg, sizeof(msg),
                     acc::strings::Get(acc::strings::Id::FmtLevelUpDoStepFirst),
                     name);
            acc::speech::Speak(msg, /*interrupt=*/false);
            return;
        }
    }
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::LevelUpStepLocked),
                 /*interrupt=*/false);
}

void ResetTabbedState() {
    g_tabbedPanel = nullptr;
    g_tabsStart   = -1;
    g_tabsCount   = 0;
}

void RebindChainPreserveIndex(void* panel) {
    int savedIndex = g_chainIndex;
    RebindChain(panel);
    if (g_chainCount <= 0) {
        g_chainIndex = 0;
        return;
    }
    if (savedIndex < 0) savedIndex = 0;
    if (savedIndex >= g_chainCount) savedIndex = g_chainCount - 1;
    g_chainIndex = savedIndex;
}

void InvalidateChain() {
    g_chainPanel  = nullptr;
    g_chainVtable = 0;
    g_chainIndex = 0;
    g_chainCount = 0;
    // Tabbed-panel state is intentionally NOT reset here. The chain panel and
    // the tabbed panel are orthogonal: closing a sub-panel (Graphics) only
    // needs to invalidate the chain, but used to also wipe g_tabbedPanel for
    // the still-live parent (Options), forcing MaybeDetectTabs to re-latch
    // on every reopen — leaving the click-sim/warp offset stale in the
    // window between rebind and re-detect (patch-20260530-110829.log,
    // line 374-511). ValidateTabbedPanel handles tabbed-panel liveness
    // independently each tick.
}

void ValidateTabbedPanel() {
    if (!g_tabbedPanel) return;
    if (IsPanelLive(g_tabbedPanel)) return;
    acclog::Write("ValidateTabbedPanel", "%p not in panels[]; clearing tabbed-mode state",
                  g_tabbedPanel);
    ResetTabbedState();
}

// Read a panel/control's vtable word under SEH. 0 on a faulting read or a
// null object — callers treat 0 as "cannot vouch for this object".
uintptr_t ReadVtable(void* obj) {
    if (!obj) return 0;
    uintptr_t vt = 0;
    __try {
        vt = *reinterpret_cast<uintptr_t*>(obj);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return vt;
}

// Identity check for the panel the chain is bound to.
//
// IsPanelLive (below) asks "is this pointer in panels[]", which a freed
// panel can pass again the moment the allocator hands its block to a NEW
// panel — the pointer looks live because something else now lives there.
// KOTOR 2 chargen hit exactly that: the Abilities panel was freed, we kept
// dispatching its buttons (their is_active read as garbage — 2622030025 in
// patch-20260819-221733), and the Skills panel was then allocated onto the
// same block, inheriting a half-written creature field. The engine's own
// handlers then walked that field and took the process down.
//
// Comparing the vtable we recorded at rebind against the one at that
// address now catches the reuse-by-a-different-class case cheaply: same
// pointer, different class, so the object is not what we bound.
//
// NOT covered: reuse by a panel of the SAME class. That needs a generation
// token; this lands first because it is falsifiable on the known repro.
bool ChainPanelIdentityHolds() {
    if (!g_chainPanel || g_chainVtable == 0) return true;  // nothing vouched
    uintptr_t now = ReadVtable(g_chainPanel);
    if (now == g_chainVtable) return true;
    acclog::Write("ChainIdentity",
                  "STALE panel=%p bound-vtable=0x%p now=0x%p — the address "
                  "was reused by a different panel class; invalidating "
                  "instead of driving it",
                  g_chainPanel, (void*)g_chainVtable, (void*)now);
    return false;
}

void ValidateChainPanel() {
    if (!g_chainPanel) return;
    if (!ChainPanelIdentityHolds()) {
        InvalidateChain();
        return;
    }
    if (IsPanelLive(g_chainPanel)) return;
    acclog::Write("ValidateChainPanel", "%p not in panels[]; invalidating chain",
                  g_chainPanel);
    InvalidateChain();
}

// ============================================================================
// RebindChain — the heart of chain navigation. Walks panel.controls,
// recurses into sub-dialog listboxes, sorts by visual y, squashes
// cycle-arrow flankers, computes click-offset compensations, anchors the
// cursor on the engine's current activeControl.
// ============================================================================

void RebindChain(void* panel) {
    g_chainPanel  = panel;
    g_chainVtable = ReadVtable(panel);
    g_chainIndex  = 0;
    g_chainCount  = 0;
    if (!panel) return;

    // First-sight walk + cycle-category capture, normally driven by
    // OnSetActiveControl. A panel can reach the user's foreground without
    // the engine ever firing SetActiveControl on it — KOTOR 2 pushes every
    // chargen wizard step (abchrgen_p, skchrgen_p, portcust_p) straight out
    // of the parent step-list's button handler, so the walk never ran and
    // the routing layer logged "fg=<step> current=<parent>" for the whole
    // visit. The visible cost was the Attributes / Skills rows speaking
    // a bare "8" instead of "Strength, 8": CaptureLabels lives at the end of
    // this walk and binds ability_labels[i] onto ability_buttons[i].
    //
    // Placed ahead of the control walk below so the capture is in the cache
    // before this rebind's own FromControl calls (squash + chain dump) read
    // it. Idempotent — WalkAndCaptureOnFirstSight self-guards on a
    // last-panel latch, so on KOTOR 1 (where SetActiveControl already ran
    // for this panel) it is an immediate no-op.
    acc::menus::focus::WalkAndCaptureOnFirstSight(panel);

    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return;
    int n = list->size > 256 ? 256 : list->size;

    bool  modalText           = IsModalTextPanel(IdentifyPanel(panel));
    void* portraitChargenSkip = ResolvePortraitChargenSkip(panel);
    void* equipPickerLb       = ResolveEquipPickerListBox(panel);
    void* workbenchUpgradeLb  = ResolveWorkbenchUpgradeListBox(panel);

    void* storeCancelBtn = nullptr;
    void* storeToggleBtn = nullptr;
    void* storeAcceptBtn = nullptr;
    ResolveStoreActionButtons(panel, storeCancelBtn, storeToggleBtn,
                              storeAcceptBtn);

    // K2 crafting screens: the same trade as the store's action buttons
    // above. Q/E swaps the create list for the break-down list and Enter on a
    // row commits it, so both buttons would be stops that repeat what a key
    // already does.
    void* craftViewToggleBtn = acc::menus::crafting::ViewToggleButton(panel);
    void* craftCommitBtn     = acc::menus::crafting::CommitButton(panel);

    // Resolve the engine's localized close caption (strref 1582 →
    // "Close"/"OK"/…) once per rebind, for the universal
    // close-button filter in isDecorative below. Empty on TLK miss, which
    // disables the filter (fail-open — never hides a real action button).
    char closeCaption[64];
    bool haveCloseCaption =
        LookupTlk(kCloseButtonStrRef, closeCaption, sizeof(closeCaption)) &&
        closeCaption[0] != '\0';

    // Credits row FIRST, before the control/listbox walk: Inventory's item
    // listbox can append 60+ entries and fill the chain to kMaxChainEntries,
    // and a credits row queued afterwards silently lost its slot to the cap.
    // The cy-sort below still lands it at the top via its synthetic sortCy.
    acc::menus::credits::ForEachCreditsRowAnchor(panel, OnCreditsAnchor, panel);

    // Top of the highest listbox block that contributes rows to this chain.
    // Stays INT_MAX on panels without one. See DemoteFilterStripAboveList.
    int listBlockTop = INT_MAX;

    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c) continue;
        if (c == portraitChargenSkip) continue;
        if (c == storeCancelBtn || c == storeToggleBtn ||
            c == storeAcceptBtn) continue;
        if (c == craftViewToggleBtn || c == craftCommitBtn) continue;

        if (IsChainNavigable(c)) {
            if (IsDecorativeControl(panel, c, closeCaption, haveCloseCaption)) {
                continue;
            }
            AppendChainEntry(c);
            continue;
        }
        AppendListBoxChildren(panel, c, equipPickerLb, workbenchUpgradeLb,
                              modalText, listBlockTop);
    }

    // The remaining virtual rows. Each ForEach*Anchor is a no-op on panel
    // kinds it does not own, except the charsheet one which is gated here.
    if (IdentifyPanel(panel) == PanelKind::InGameCharacter) {
        acc::menus::charsheet::ForEachStatRowAnchor(panel, OnStatRowAnchor,
                                                    panel);
    }
    AppendStatusSummaryRows(panel, list, n);
    acc::menus::extract::ForEachWagerRowAnchor(panel, OnWagerAnchor, panel);
    acc::menus::equipstats::ForEachEquipStatRowAnchor(
        panel, OnEquipStatAnchor, panel);
    acc::menus::modsettings::ForEachRootAnchor(panel, OnModSettingsAnchor,
                                               panel);

    // Runs on the finished entry set (the virtual-row anchors above pick their
    // own synthetic sortCy and are non-geometric, so they are never demoted),
    // and before the sort that consumes sortCy.
    DemoteFilterStripAboveList(panel, listBlockTop);

    SortChainBySortCy();
    SquashCycleFlankers(panel);

    // Tab-cluster Y offset is computed on demand at warp/click-sim time via
    // ComputeTabClickOffsetY — see the header comment for the race that made
    // an eager rebind-time computation unreliable.
    ComputeEquipSlotClickOffset(panel);
    ComputeClassIconClickOffset(panel);

    // Anchor at active.
    void* active = ReadPanelActiveControl(panel);
    int   idx    = FindChainEntry(active);
    g_chainIndex = (idx >= 0) ? idx : 0;

    // Mirror initial chain focus into the chargen Attributes panel's
    // selected_ability so a Left/Right press immediately after the panel
    // opens (without first stepping with Up/Down) modifies the right
    // ability. No-op on every other panel. The chain-step handler in
    // menus.cpp keeps the field in sync as the user navigates.
    acc::menus::chargen_attr::SyncSelectedAbilityFromChainFocus();
    // Same on the Skills panel.
    acc::menus::chargen_skills::SyncSelectedSkillFromChainFocus();

    LogChainContents(panel, active);
}

}  // namespace acc::menus::chain
