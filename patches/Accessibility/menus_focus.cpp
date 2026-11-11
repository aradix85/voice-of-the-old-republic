// First-sight / focus-capture logic for the panel focus hook:
// OnSetActiveControl (menus_setactive.cpp) calls these five functions; see
// menus_focus.h for what each one owns. The pending-announce slot is
// menus.cpp's, written here through the externs in menus_internal.h.

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "log.h"
#include "speech.h"
#include "menus.h"
#include "menus_focus.h"
#include "menus_internal.h"
#include "menus_extract.h"
#include "menus_chain.h"
#include "menus_monitors.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_skills.h"
#include "menus_powers_levelup.h"
#include "menus_editbox.h"
#include "menus_listbox.h"
#include "menus_crafting.h"  // GetTitleOverride — K2 crafting screens
#include "menus_store.h"
#include "engine_input.h"
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "engine_addr.h"
#include "strings.h"
#include "tutorial_hints.h"
#include "tutorial_popup.h"
#include "diag_chargen_feats.h"
#include "focus_guard.h"

using namespace acc::engine;

using acc::menus::detail::IsClassSelectionIcon;
using acc::menus::detail::ClassLabelCacheLookup;
using acc::menus::detail::ClassLabelCacheStore;
using acc::menus::chain::WalkChildren;
using acc::menus::chain::FindAdjacentArrow;

// The last panel whose title was spoken. Focus coming back to the same panel
// pointer must not announce it again.
static void* g_lastTitledPanel = nullptr;

namespace {

// A title one of the screen modules supplies itself, where the first label of
// the .gui file is wrong (a placeholder, or the header of another screen).
// `source` names the module for the log.
const char* TitleOverride(void* panel, const char** source) {
    if (const char* title = acc::menus::listbox::GetTitleOverride(panel)) {
        *source = "spec";
        return title;
    }
    if (const char* title = acc::menus::editbox::GetTitleOverride(panel)) {
        *source = "editbox spec";
        return title;
    }
    if (const char* title = acc::menus::powers_levelup::GetTitleOverride(panel)) {
        *source = "powers_levelup";
        return title;
    }
    return nullptr;
}

// On a fresh launch the engine reaches the main menu with its DirectInput
// keyboard unacquired: menu input is dead until an alt-tab forces a
// SetActive(0) -> (1) edge. The main menu's first sight is the first moment
// the game is certainly interactive, so the edge is made here.
//
// It has to be the full 0 -> 1 cycle. On some machines the engine arrives
// with the active flag a stale 1 (set, devices not acquired) and a bare
// SetActive(1) does nothing against it.
//
// Only while the game owns the foreground: input mirrors the foreground, and
// acquiring behind another window would send its keys into the game. If the
// game is not in front yet, the focus-regain edge acquires when it is.
void WakeInputAtMainMenu() {
    acclog::BringupMark("main_menu_first_sight");
    if (acc::focus_guard::GameOwnsForeground()) {
        acc::engine::ForceReacquireInput();
    } else {
        acclog::Write("EngineInput",
            "cold-start acquire skipped: game not foreground at "
            "first-sight; the focus-regain edge will acquire");
    }
    // Normally started at rules init already; a second start is a no-op.
    acc::focus_guard::StartFocusProbe();
}

// Why a label's text cannot be a panel title, or nullptr when it can.
const char* NotATitle(const char* text) {
    // Tab indicators ("1".."6") and stat values: up to three digits.
    size_t digits = 0;
    while (text[digits] >= '0' && text[digits] <= '9') ++digits;
    if (digits > 0 && digits <= 3 && text[digits] == '\0') return "short numeric";

    // KOTOR 2's chargen Feats panel leads with a leftover of the container
    // screen, "Items Available to Place in Container and blah blah blah",
    // untranslated in every localisation: matched by its tail.
    if (strstr(text, "blah blah blah") != nullptr) return "dev placeholder";
    return nullptr;
}

// The panel's first label with text that can be a title. Buttons and other
// interactive controls are left to the focus announcements.
void SpeakFirstLabelAsTitle(void* panel) {
    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return;
    const int n = list->size > 256 ? 256 : list->size;
    for (int i = 0; i < n; ++i) {
        void* child = list->data[i];
        if (!child) continue;
        if (CallDowncast(child, kVtableAsLabel) == nullptr &&
            CallDowncast(child, kVtableAsLabelHilight) == nullptr) {
            continue;
        }
        char text[256];
        if (!acc::menus::extract::FromControl(child, text, sizeof(text), panel)) continue;
        if (const char* why = NotATitle(text)) {
            acclog::Write("Menus.PanelWalk",
                          "title parent=%p label=%p text=\"%s\" (skipped: %s)",
                          panel, child, text, why);
            continue;
        }
        // Do NOT filter on the visible bit. On KOTOR 2 the flag word at
        // kControlBitFlagsOffset reads garbage for labels (it is right for
        // buttons on both games), and with the filter every real title,
        // "Load Game" included, was dropped as invisible. The flags are
        // logged so that a later attempt has data.
        acclog::Write("Menus.PanelWalk",
                      "title parent=%p label=%p text=\"%s\" bit_flags=0x%x",
                      panel, child, text, ReadControlBitFlags(child));
        acc::speech::Speak(text, /*interrupt=*/false);
        return;
    }
}

}  // namespace

// The first focus into a panel speaks its title, so that the user knows
// which menu this is. With no usable label it stays silent and the
// announcement of the focused control has to do.
static void AnnouncePanelTitle(void* panel) {
    if (!panel) return;

    // One dump of the CSWGuiFeatsCharGen structure per panel pointer; nothing
    // for other panels.
    acc::diag::chargen_feats::DumpStructureIfNeeded(panel);

    // KOTOR 2's workbench pushes Select, Items and Upgrade stacked in one
    // tick and reuses the panels afterwards, so first sight would speak
    // several titles at once and never again. menus_crafting announces them
    // on the foreground edge instead.
    if (acc::menus::crafting::OwnsPanelTitle(panel)) {
        acclog::Write("Menus.PanelWalk",
                      "title parent=%p suppressed (crafting fg-edge owns it)", panel);
        return;
    }

    const char* source = nullptr;
    const char* title = TitleOverride(panel, &source);
    if (!title && IdentifyPanel(panel) == PanelKind::MainMenu) {
        // The main menu's only label is the optional Steam notice about
        // downloadable content, which the label walk would take.
        WakeInputAtMainMenu();
        title = acc::strings::Get(acc::strings::Id::PanelTitleMainMenu);
        source = "main menu";
    }
    if (title) {
        acclog::Write("Menus.PanelWalk", "title parent=%p (%s override) text=\"%s\"",
                      panel, source, title);
        acc::speech::Speak(title, /*interrupt=*/false);
        return;
    }
    SpeakFirstLabelAsTitle(panel);
}

// Chargen class-icon cache pump. The hook fires at the very entry of
// CSWGuiPanel::SetActiveControl — before the function writes the new
// active_control and before any OnEnterButton runs for the new icon. At
// this moment panel.active_control still points at the OUTGOING icon
// (the one the user is leaving) and class_label still carries that
// icon's class string. Caching (outgoing -> label) on every transition
// closes the gap that the per-frame monitor leaves under rapid arrow
// input: the monitor only catches icons the user dwells on, but every
// SetActiveControl event fires regardless of duration.
void acc::menus::focus::PrefillClassIconCacheOnTransition(void* panel, void* newControl) {
    if (!acc::engine::HasVtable(panel, kVtableCSWGuiClassSelection)) return;
    void* outgoing = *reinterpret_cast<void**>(
        reinterpret_cast<unsigned char*>(panel) + kPanelActiveControlOffset);
    if (!outgoing || outgoing == newControl) return;
    if (!IsClassSelectionIcon(panel, outgoing)) return;
    if (ClassLabelCacheLookup(panel, outgoing) != nullptr) return;
    void* classLabel = reinterpret_cast<unsigned char*>(panel) +
                       kClassSelectionClassLabelOffset;
    char name[256];
    if (ExtractTextOrStrRefIndirect(classLabel,
                                    kLabelTextOffset,
                                    kLabelStrRefOffset,
                                    kLabelTextObjectOffset,
                                    name, sizeof(name)) &&
        name[0] != '\0') {
        // Compose the description in the same breath. The cache is
        // first-write-wins, so a prefill that stored the name alone would
        // lock the entry and the blurb could never be added later.
        char text[acc::menus::detail::kAnnounceTextMax];
        acc::menus::detail::ComposeClassAnnounce(panel, name, text, sizeof(text));
        ClassLabelCacheStore(panel, outgoing, text);
        acclog::Write("Menus.PerKind",
                      "ClassSelection prefill outgoing=%p -> \"%s\"",
                      outgoing, text);
    }
}

// Update g_currentPanel on panel transitions. Tabbed-mode tab-cluster
// state DELIBERATELY persists across transitions into sub-dialogs of the
// tabbed panel — the tab strip lives on the parent panel (Options) and is
// still the right thing while the user is inside one of its sub-dialogs.
// ValidateTabbedPanel drops the cluster state when the engine frees the
// underlying panel.
void acc::menus::focus::UpdateFocusedPanelState(void* panel) {
    g_currentPanel = panel;
}

// First focus event into a previously-unseen panel: dump every child
// control + capture cycle-button category text. Cycle widgets carry their
// localized category in their CExoString at panel construction time; the
// engine replaces it with the persisted value (e.g. "Difficulty" -> "Normal")
// shortly after, and our FireActivate calls overwrite it again on each
// cycle. SetActiveControl's first fire on a new panel is the earliest
// reachable capture point.
void acc::menus::focus::WalkAndCaptureOnFirstSight(void* panel) {
    static void* s_lastPanel = nullptr;
    if (!panel || panel == s_lastPanel) return;
    s_lastPanel = panel;

    LogManagerStack(*reinterpret_cast<void**>(kAddrGuiManagerPtr),
                    "panel-walk");
    PanelKind kind = IdentifyPanel(panel);
    // Header (panel=/kind=) is folded into the walk block by WalkChildren so
    // the whole snapshot collapses as one unit on identical re-walks.
    WalkChildren("Menus.PanelWalk", panel, kPanelControlsOffset,
                 PanelKindName(kind));

    // Capture cycle-button categories before any activation rewrites the
    // value-display button's CExoString. The cache lives in
    // menus_extract.cpp; we write through its public setter.
    acc::menus::extract::ResetCycleCategoryCache();
    auto* plist = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!plist || !plist->data || plist->size <= 0) return;

    int pn = plist->size > 256 ? 256 : plist->size;
    for (int i = 0; i < pn; ++i) {
        void* c = plist->data[i];
        if (!c) continue;
        if (CallDowncast(c, kVtableAsButton) == nullptr) continue;
        if (IsToggle(c)) continue;
        void* leftN  = FindAdjacentArrow(panel, c, /*toRight=*/false);
        void* rightN = FindAdjacentArrow(panel, c, /*toRight=*/true);
        if (!leftN && !rightN) continue;

        char text[128];
        bool gotText = false;
        uint32_t strref = ReadU32Or0(c, kButtonStrRefOffset);
        if (LookupTlk(strref, text, sizeof(text))) {
            gotText = true;
        } else if (ReadCExoString(c, kButtonTextOffset, text, sizeof(text))) {
            gotText = true;
        }
        if (gotText) {
            acc::menus::extract::CaptureCycleCategory(c, text);
            acclog::Write("Menus.CycleCategory", "control=%p text=\"%s\" strref=%u",
                          c, text, strref);
        }
    }

    // Chargen "Attribute" panel: each value button has an inline text "8"
    // (the current value), so the generic capture above resolves the
    // category to the value itself (useless). Override here by reading
    // ability_labels[i] for each ability_buttons[i] and binding the pair
    // into the cycle-category cache so FromControl produces "Strength, 8".
    acc::menus::chargen_attr::CaptureLabelsIfApplicable(panel);

    // Same for the Skills panel — value buttons all read "0" initially
    // and would otherwise resolve to "0" as the category. Bind the
    // skill_labels[i] text in instead.
    acc::menus::chargen_skills::CaptureLabelsIfApplicable(panel);

    // Chargen class-selection panel: snapshot LBL_DESC's authored text
    // before the user hovers anything. Both games ship it with a
    // placeholder (empty on KOTOR 1, "This is just a test line." repeated
    // on KOTOR 2) and fill in the real class description on hover, so this
    // snapshot is what lets the announce tell content from default. This
    // walk is the last moment that is guaranteed to precede the engine's
    // first OnEnterButton.
    if (acc::engine::HasVtable(panel, kVtableCSWGuiClassSelection)) {
        void* descLabel = acc::menus::detail::FindControlById(panel, kClassSelDescLabelId);
        char desc[384] = {0};
        if (descLabel) {
            acc::engine::ReadLabelText(descLabel, desc, sizeof(desc));
        }
        acc::menus::detail::ClassDescBaselineCapture(panel, desc);
        acclog::Write("Menus.PerKind",
                      "ClassSelection LBL_DESC baseline label=%p text=\"%.120s\"",
                      descLabel, desc);
    }
}

// First focus into a new panel: speak its title once. The focused
// control's announcement still fires after, so the user hears
// "<panel title>, <focused control>" on entry.
//
// In-game sub-screens (Inventory, Map, Options, …) are skipped here:
// AnnounceNewSubScreens in menus_monitors.cpp announces them with the
// TLK-localized strref name on push. Doing both produces "Options,
// Save Game, Options" on Options open (panel title + focus +
// sub-screen monitor) instead of the intended "Options, Save
// Game" sequence.
void acc::menus::focus::SpeakPanelTitleOnFirstSight(void* panel) {
    if (!panel || panel == g_lastTitledPanel) return;
    g_lastTitledPanel = panel;
    PanelKind kind = IdentifyPanel(panel);
    if (acc::menus::monitors::IsInGameSubScreenKind(kind)) {
        return;
    }
    // Dialog panels: dialog_speech.cpp is the authoritative announcer
    // (with race/language suppression). The generic label-walk would
    // speak the dialog text as the "title" the first time SetActive
    // fires on the panel — bypassing suppression.
    if (kind == PanelKind::DialogCinematicCopy ||
        kind == PanelKind::DialogLetterbox1 ||
        kind == PanelKind::DialogLetterbox2 ||
        kind == PanelKind::DialogLetterbox3) {
        return;
    }
    AnnouncePanelTitle(panel);
}

// Record the new focused control into the pending-announce slot for the
// drain to read on the next tick. Logs the event diagnostically — the
// announce itself happens in DrainPendingAnnounce. Container-listbox
// suppression stays at write-time: drowning the per-row container monitor
// would still be wrong, so we just don't queue.
void acc::menus::focus::AnnounceNewFocusedControl(int n, void* panel, void* newControl) {
    // Mapped tutorial popups (Surface 1) and our synthetic Trask popup (Surface
    // 2): the message body is spoken as a keyboard hint by the content-fingerprint
    // monitor, and the buttons read on arrow-nav. Suppress the engine-focus-driven
    // announce of the raw control here so neither the mouse body nor a button
    // (e.g. "Cancel") is read on the popup's first-sight SetActiveControl.
    if (IdentifyPanel(panel) == PanelKind::TutorialBox &&
        (acc::tutorial_popup::SyntheticActive() ||
         acc::tutorial_hints::HintForTutorialRow(
             acc::tutorial_hints::ReadTutorialRow(panel)) != nullptr)) {
        return;
    }

    int id = ReadControlId(newControl);

    char text[256];
    const char* source = acc::menus::extract::FromControl(
        newControl, text, sizeof(text), panel);

    bool suppressForContainer =
        IsListBox(newControl) &&
        IdentifyPanel(panel) == PanelKind::Container;

    // Store mode flip (ShowBuyGUI / ShowSellGUI) ends with the engine
    // calling SetActiveControl on whichever item-listbox just became
    // visible. FromControl on a listbox returns the concatenation of
    // every row label — for a merchant with 30 items that's a full
    // inventory dump after every G press. We already announce "Buy
    // mode" / "Sell mode" from TickMonitorMode and rebind the
    // chain; the user can Up/Down to hear individual items. Suppress
    // the listbox-blob speech the same way Container does.
    bool suppressForStore =
        IsListBox(newControl) &&
        acc::menus::store::IsStorePanel(panel);

    if (source) {
        acclog::Write("Menus.SetActive", "#%d panel=%p new=%p id=%d src=%s text=\"%s\"",
                      n, panel, newControl, id, source, text);
    } else {
        char vtbl[160];
        DumpControlVtable(newControl, vtbl, sizeof(vtbl));
        acclog::Write("Menus.SetActive", "#%d panel=%p new=%p id=%d src=none %s",
                      n, panel, newControl, id, vtbl);
    }

    if (suppressForContainer || suppressForStore) return;

    // Cross-panel overwrite guard. The slot-collapse model assumes the
    // intra-tick burst stays on one panel — engine fires NULL → first →
    // settled on the just-opened panel, last write wins, drain speaks the
    // settled control. But MessageBox-open same-tick also fires a follow-up
    // SetActive on the underlying panel's listbox (engine refresh of the
    // now-visible row). Without this guard that fourth fire overwrites the
    // MessageBox's Cancel with the listbox's row text, which dedups
    // against what was already spoken and the popup announces nothing.
    //
    // Fix: when the new SetActive lands on a different panel than the
    // current pending, flush the previous pending first. Same-panel bursts
    // still collapse (the common case stays cheap). Drain re-uses the full
    // path including chain-coherence drop.
    if (acc::menus::s_pendingAnnouncePanel != nullptr &&
        acc::menus::s_pendingAnnouncePanel != panel) {
        acc::menus::DrainPendingAnnounce();
    }

    acc::menus::s_pendingAnnouncePanel   = panel;
    acc::menus::s_pendingAnnounceControl = newControl;
}

