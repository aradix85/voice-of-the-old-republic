// Opening and closing the unified action menu, and keeping it on the right target while it is open.

#include "unified_action_menu_internal.h"

#include "combat.h"             // IsPartyInCombat — out-of-combat fire-and-close
#include "engine_picker.h"      // ReanchorRadial (per-press target re-anchor)
#include "engine_subscreen.h"   // Begin/EndOverlayPause
#include "log.h"
#include "strings.h"
#include "speech.h"

namespace acc::unified_menu {

bool ArmFromRadial(const char* name, uint32_t targetHandle) {
    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    if (!tam) {
        acclog::Write("UnifiedMenu", "ArmFromRadial — TAM unresolved; not arming");
        return false;
    }
    // Preserve Shift+Enter semantics: only arm when a target category is
    // populated. No target rows → return false so the caller speaks the
    // existing "No actions available … Enter" redirect (no regression).
    int firstRow = FirstPopulatedTargetRow(tam);
    if (firstRow < 0) {
        acclog::Write("UnifiedMenu", "ArmFromRadial — no populated target row; "
            "not arming (caller speaks no-actions)");
        return false;
    }
    void* mi = acc::engine_actionbar::ResolveMainInterface();

    Arm();
    g.hasTargetBlock = true;
    g.targetHandle = targetHandle;
    g.creature = DetectCreature(tam, targetHandle);
    std::snprintf(g.targetName, sizeof(g.targetName), "%s",
                  (name && name[0]) ? name : "");

    BuildCategoryList(tam, mi);
    int loc = LocateCat(CatKind::Target, firstRow);
    g.curCat = (loc >= 0) ? loc : 0;

    char prefix[160] = "";
    std::snprintf(prefix, sizeof(prefix),
                  acc::strings::Get(acc::strings::Id::FmtInteractRadial),
                  g.targetName);

    acclog::Write("UnifiedMenu", "ARMED (radial) target=0x%08x creature=%d "
        "cats=%d curCat=%d", targetHandle, g.creature ? 1 : 0,
        g.catCount, g.curCat);
    SpeakCategory(tam, mi, prefix);
    return true;
}

bool OpenTarget(int row) {
    if (row < 0 || row >= kRowCount) return false;
    if (ForegroundPanelBlocks()) {
        acclog::Write("UnifiedMenu", "OpenTarget row=%d — foreground panel; not arming",
            row);
        return false;
    }
    uint32_t handle = ResolveNarratedServerHandle();
    if (handle == 0) {
        // No focused target — Shift+1/2/3 are explicit target openers, so
        // say "No object focused" rather than dropping into the personal block.
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::GuidanceNoFocus),
                     /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "OpenTarget row=%d — no narrated target", row);
        return false;
    }
    // Capture handle + requested row in State before any engine call — the
    // engine-call chain corrupts stack locals (see State::reqSlot).
    g.targetHandle = handle;
    g.reqSlot = row;

    // Read the LIVE target-action menu as-is — do NOT re-populate here. The
    // bare-key path in input_pipeline already re-targets + re-populates BOTH
    // blocks against this same target on the Shift+1..3 press (PopulateMenus
    // rebuilds the target rows too), in the engine's own input-dispatch
    // context. Re-populating again from this tick context is what made the
    // engine synthesise a phantom confirm that fired the menu's first entry
    // (see OpenPersonal note + the 06:55 vs 10:44 logs, 2026-06-07).
    void* tam = acc::engine_radial::ResolveTargetActionMenu();
    void* mi  = acc::engine_actionbar::ResolveMainInterface();
    if (!tam || acc::engine_radial::RowActionCount(tam, g.reqSlot) <= 0) {
        char msg[128];
        std::snprintf(msg, sizeof(msg),
                      acc::strings::Get(acc::strings::Id::FmtActionBarColumnEmpty),
                      g.reqSlot + 1);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "OpenTarget row=%d empty (target=0x%08x)",
            g.reqSlot, g.targetHandle);
        // A menu still armed from an earlier open (possibly against a
        // DIFFERENT target) must not survive this refusal: it kept its old
        // categories navigable after "Column N is empty", so the user
        // browsed and fired the OLD target's menu believing it was the new
        // one (the 13:20 Machtbruch-on-Malak in patch-20260717-131859.log).
        // Worse, the g.targetHandle capture above had already clobbered the
        // armed menu's target. Close it outright — matching the "all
        // categories empty" disarms — so the refusal leaves no ghost menu.
        if (g.active) {
            ForceDisarm("open-empty-row");
        } else {
            g.targetHandle = 0;   // not arming — don't leave a stale target
        }
        return false;
    }

    Arm();
    g.hasTargetBlock = true;
    g.creature = DetectCreature(tam, g.targetHandle);
    g.targetName[0] = '\0';

    BuildCategoryList(tam, mi);
    int loc = LocateCat(CatKind::Target, g.reqSlot);
    g.curCat = (loc >= 0) ? loc : 0;

    acclog::Write("UnifiedMenu", "ARMED (target) row=%d target=0x%08x creature=%d "
        "cats=%d", g.reqSlot, g.targetHandle, g.creature ? 1 : 0, g.catCount);
    SpeakCategory(tam, mi, /*prefix=*/nullptr);
    return true;
}

bool OpenPersonal(int col) {
    if (col < 0 || col >= kColumnCount) return false;
    if (ForegroundPanelBlocks()) {
        acclog::Write("UnifiedMenu", "OpenPersonal col=%d — foreground panel; not arming",
            col);
        return false;
    }
    // Capture the requested column in State before any engine call — the
    // engine-call chain corrupts stack locals (see State::reqSlot).
    g.reqSlot = col;

    // Shift+4..7 is a PERSONAL entry point, but the menu is meant to be
    // TRULY unified: if a hostile target is in focus we fold its three target
    // rows (Attacks / Force Powers / Items) in alongside the personal
    // columns, so Left/Right crosses the personal⇄target border freely. The
    // cursor still lands on the requested personal column; the target
    // categories sit one Left away — parity with Shift+1..3 / Shift+Enter.
    // With no focused target the menu stays personal-only, so self-buffs
    // (Shift+5 medpac, stims) still work without an enemy.
    //
    // Read the LIVE main-interface + target-action menu as-is — do NOT call
    // PrepareBareDispatch / RePopulateMainInterface here. The bare-key path in
    // input_pipeline already re-targeted + re-populated BOTH blocks against
    // the narrated target on this same Shift+number press, in the engine's own
    // input-dispatch context. Re-populating again from this tick context made
    // the engine synthesise a phantom confirm one tick later that fired the
    // menu's first entry (verified against the 06:55 vs 10:44 logs, 2026-06-07;
    // see the identical note in OpenTarget).
    void* mi = acc::engine_actionbar::ResolveMainInterface();

    if (!mi) {
        acclog::Write("UnifiedMenu", "OpenPersonal col=%d — main interface "
            "unresolved; not arming", g.reqSlot);
        return false;
    }

    // The requested column may be empty — Shift+4 self-powers on a non-Jedi or
    // a droid, Shift+6 explosives with no grenades. Say so by name, then open
    // anyway and land on the first populated category.
    //
    // Refusing outright was the old rule ("never silently jump to a different
    // category"), and it read as a menu that would not respond: on T3-M4 every
    // open announced "Own Force Powers: empty" and then nothing at all
    // answered the arrow keys, because nothing had opened. The rule's real
    // intent was don't jump SILENTLY, and this notice is what makes it not
    // silent. It rides in as SpeakCategory's prefix rather than as its own
    // utterance, because SpeakCategory speaks with interrupt=true and would
    // otherwise cancel it — the user hears "<column>: empty. <where you
    // landed>" as one line.
    char emptyNotice[160] = "";
    if (acc::engine_actionbar::VariantCount(mi, g.reqSlot) <= 0) {
        Cat reqCat{CatKind::Personal, g.reqSlot};
        const char* name = CategoryName(reqCat);
        if (name && name[0]) {
            std::snprintf(emptyNotice, sizeof(emptyNotice),
                acc::strings::Get(acc::strings::Id::FmtMenuCategoryEmpty), name);
        } else {
            std::snprintf(emptyNotice, sizeof(emptyNotice),
                acc::strings::Get(acc::strings::Id::FmtActionBarColumnEmpty),
                g.reqSlot + 1);
        }
        acclog::Write("UnifiedMenu", "OpenPersonal col=%d empty -> landing on "
            "first populated category [%s]", g.reqSlot, emptyNotice);
    }

    // Fold in the target block when a live hostile target is focused and its
    // rows are populated. Mirrors OpenTarget's resolution, minus the row
    // requirement: here the personal column is the entry point, so an absent /
    // unpopulated target simply means personal-only (tam stays null).
    uint32_t handle = ResolveNarratedServerHandle();
    void* tam = nullptr;
    if (handle != 0) {
        void* t = acc::engine_radial::ResolveTargetActionMenu();
        if (t && FirstPopulatedTargetRow(t) >= 0) tam = t;
    }
    const bool hasTarget = (tam != nullptr);

    g.hasTargetBlock = hasTarget;   // false → no per-press re-anchor
    g.targetHandle   = hasTarget ? handle : 0;
    g.creature       = hasTarget ? DetectCreature(tam, handle) : false;
    g.targetName[0]  = '\0';

    // Build BEFORE arming. With nothing populated anywhere there is no menu to
    // open, and arming first would take a world pause only to hand it straight
    // back.
    BuildCategoryList(tam, mi);  // tam==nullptr → personal block only
    if (g.catCount == 0) {
        if (emptyNotice[0]) acc::speech::Speak(emptyNotice, /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "OpenPersonal col=%d — no populated "
            "category anywhere; not arming", g.reqSlot);
        return false;
    }

    Arm();

    // Land on the requested column when it is populated; otherwise on the
    // first category there is. Left/Right reach the rest, including the target
    // rows when folded in.
    int loc = LocateCat(CatKind::Personal, g.reqSlot);
    g.curCat = (loc >= 0) ? loc : 0;

    acclog::Write("UnifiedMenu", "ARMED (personal) col=%d hasTarget=%d "
        "target=0x%08x creature=%d cats=%d curCat=%d", g.reqSlot,
        hasTarget ? 1 : 0, g.targetHandle, g.creature ? 1 : 0,
        g.catCount, g.curCat);
    SpeakCategory(tam, mi, emptyNotice[0] ? emptyNotice : nullptr);
    return true;
}

// Steps of HandleInputEvent below. They are file-local because every one of
// them writes `g` and is only meaningful inside a single input dispatch —
// the HARD RULE is that the menu is populated from input_pipeline and
// nowhere else, so none of this may become reachable from OnUpdate or
// PollHotkey.

// Esc — close. When we held the world pause, ForceDisarm → EndOverlayPause
// resumes the world and the engine's pause-resume cue ("Unpaused")
// is the close announcement (no extra phrase — it was redundant and
// misleading: Esc closes the menu, it doesn't cancel; queued actions stay
// queued and run on resume). When the Action Menu auto-pause option is off
// we never paused, so there is no resume cue — speak an explicit close
// confirmation instead so the user hears the menu dismissed.
void CloseFromEsc() {
const bool wasPaused   = g.pausedOnOpen;
const bool outOfCombat = !acc::combat::IsPartyInCombat();
ForceDisarm("esc");
if (outOfCombat) {
    // Native sub-screens unpause the world on Esc; the unified menu now
    // matches that out of combat. ForceDisarm already released our own
    // overlay pause (auto-pause option on) via EndOverlayPause; if the
    // world is STILL paused that is the player's own manual pause, so
    // resume it here — the engine's "Unpaused" resume cue is then the
    // close announcement. When nothing resumed (never paused, or the
    // overlay release already unpaused silently) speak an explicit close
    // confirmation so the close is never silent.
    if (!acc::engine::ResumeWorldIfPaused("unified-esc")) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ActionMenuClosed),
                     /*interrupt=*/true);
    }
} else if (!wasPaused) {
    // In combat: unchanged. Esc keeps the tactical pause exactly as
    // before — ForceDisarm → EndOverlayPause resumes only the pause we
    // owned; the encounter's own pause is untouched.
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ActionMenuClosed),
                 /*interrupt=*/true);
}
}

// FOLLOW-CYCLING re-anchor: when the narrated target changed while the
// menu sat open (`,`/`.`/Q/E/passive all stamp the same slot), rebuild
// the target rows against the NEW target so the menu always shows that
// target's real options and Enter fires at it. One re-anchor per cycle
// (the handle comparison self-quiesces), so the per-keypress churn that
// originally forced the lazy design doesn't return. The user's selected
// action follows BY IDENTITY: each row's shadow index is re-located to
// the entry with the same action_id, so cycle → Enter casts the same
// power at the next target. A row whose previous action_id is absent on
// the new target marks its selection as not-carried; Enter (only) is
// then consumed as orientation instead of firing something unheard —
// see the Enter case.
// Returns true when the menu was actually re-anchored onto the new target.
bool ReanchorToNewTarget(void* tam, uint32_t narrated,
                         bool rowCarried[kRowCount]) {
    bool targetChanged = false;
    uint32_t prevId[kRowCount];
    for (int r = 0; r < kRowCount; ++r) {
        prevId[r] = acc::engine_radial::ReadRowActionIdAtIndex(
            tam, r, g_targetSel[r]);
    }
    if (acc::picker::ReanchorRadial(narrated)) {
        uint32_t oldHandle = g.targetHandle;
        targetChanged  = true;
        g.targetHandle = narrated;
        g.creature     = DetectCreature(tam, narrated);
        if (!acc::engine_radial::ReadTargetName(
                tam, g.targetName, sizeof(g.targetName))) {
            g.targetName[0] = '\0';
        }
        for (int r = 0; r < kRowCount; ++r) {
            int ni = acc::engine_radial::FindRowIndexByActionId(
                tam, r, prevId[r]);
            rowCarried[r] = (ni >= 0);
            if (ni >= 0) g_targetSel[r] = ni;
        }
        acclog::Write("UnifiedMenu",
            "follow-cycle re-anchor 0x%08x -> 0x%08x creature=%d "
            "name=[%s] carried=%d%d%d", oldHandle, narrated,
            g.creature ? 1 : 0, g.targetName,
            rowCarried[0] ? 1 : 0, rowCarried[1] ? 1 : 0,
            rowCarried[2] ? 1 : 0);
    } else {
        acclog::Write("UnifiedMenu",
            "follow-cycle re-anchor FAILED for 0x%08x — keeping "
            "0x%08x", narrated, g.targetHandle);
    }
    return targetChanged;
}

// UNFOLD: the menu is personal-only (opened without a target, or
// its target was lost) and the user cycled onto something.
// Populate the target rows against it and fold them in — the
// mid-menu equivalent of OpenPersonal's open-time fold-in, which
// could not happen back then because nothing was narrated. Rows
// empty → target has no actions → stay personal-only and don't
// re-run the populate chain for this handle on every keypress.
// Returns true when the target block was folded in.
bool UnfoldTargetBlock(void* tam, uint32_t narrated,
                       bool rowCarried[kRowCount]) {
    bool targetChanged = false;
    if (acc::picker::ReanchorRadial(narrated)) {
        if (FirstPopulatedTargetRow(tam) >= 0) {
            targetChanged    = true;
            g.hasTargetBlock = true;
            g.targetHandle   = narrated;
            g.creature       = DetectCreature(tam, narrated);
            if (!acc::engine_radial::ReadTargetName(
                    tam, g.targetName, sizeof(g.targetName))) {
                g.targetName[0] = '\0';
            }
            g.unfoldDeclined = 0;
            // Nothing in the new rows was ever selected by the user
            // in this menu session — treat all as not-carried so an
            // Enter that somehow lands there orients instead of
            // firing. The user's personal category re-locates and
            // stays carried, so Enter there fires as normal.
            rowCarried[0] = rowCarried[1] = rowCarried[2] = false;
            acclog::Write("UnifiedMenu",
                "follow-cycle unfold target=0x%08x creature=%d "
                "name=[%s]", narrated, g.creature ? 1 : 0,
                g.targetName);
        } else {
            g.unfoldDeclined = narrated;
            acclog::Write("UnifiedMenu",
                "follow-cycle unfold declined 0x%08x — no target "
                "actions", narrated);
        }
    }
    return targetChanged;
}

// LAZY re-anchor: re-anchoring on EVERY keypress broke target-menu
// navigation (the engine's per-press re-derivation churned the rows so
// arrows produced nothing). Instead, only re-anchor to RESTORE the menu
// when the engine has actually drained our target rows out from under us
// (the cursor-coupling case, project_radial_cursor_coupling). In the
// normal paused-overlay case the rows persist, so we skip re-anchor and
// navigate the stable snapshot — exactly like the personal-only menu.
// Skipped when the follow-cycle re-anchor above already rebuilt this
// press (an actionless new target legitimately has zero rows).
void MaybeRestoreDrainedRows(void*& tam, bool targetChanged) {
    if (!targetChanged && g.hasTargetBlock && g.targetHandle != 0 && tam) {
        int t = acc::engine_radial::RowActionCount(tam, 0) +
                acc::engine_radial::RowActionCount(tam, 1) +
                acc::engine_radial::RowActionCount(tam, 2);
        if (t == 0) {
            acclog::Write("UnifiedMenu", "target rows drained — re-anchoring "
                "0x%08x", g.targetHandle);
            if (!acc::picker::ReanchorRadial(g.targetHandle)) {
                g.targetHandle = 0;  // gone for good → keep personal block
            }
            tam = acc::engine_radial::ResolveTargetActionMenu();
        }
    }
}

}  // namespace acc::unified_menu
