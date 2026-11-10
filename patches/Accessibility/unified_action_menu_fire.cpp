// Firing an action from the unified action menu: the dispatch (queued or
// replacing), the engine's refusal, and whether the menu closes afterwards.

#include "unified_action_menu_internal.h"

#include "combat.h"             // IsPartyInCombat: out-of-combat fire-and-close
#include "combat_diag.h"        // LogPreFire / LogPostFire around dispatch
#include "combat_queue.h"       // ArmUserQueueAdd: attribute the AddAction
#include "engine_player.h"      // SetLeaderQueueModeBit (append-vs-replace)
#include "engine_reads.h"       // LookupTlk
#include "engine_subscreen.h"   // WorldIsPaused
#include "hotkeys.h"            // ShiftHeld
#include "log.h"
#include "strings.h"
#include "speech.h"

namespace acc::unified_menu {

namespace {

// DoTargetAction dispatches at the creature_id baked into the
// matched list descriptor at PopulateMenus time — and while the
// menu sits open the engine re-bakes the lists against its own
// current target (combat reassert / cursor hover), so an Enter
// seconds after arming fired at the wrong object. Restamp the
// whole row with the menu's armed target right before dispatch:
// raw field writes, safe from this poll context (unlike
// RePopulate, see OpenTarget). Same client-handle convention as
// input_pipeline's PrepareBareDispatchForNarratedTarget.
void RestampTargetRowForDispatch(void* tam, const Cat& cur) {
    if (cur.kind == CatKind::Target && g.targetHandle != 0) {
        uint32_t targetClient = (g.targetHandle & 0x80000000u)
            ? g.targetHandle
            : (g.targetHandle | 0x80000000u);
        (void)acc::engine_radial::RetargetRowActions(
            tam, cur.slot, targetClient);
    }
}

// Speak the engine's own refusal, when it is about to refuse.
//
// Call BEFORE the dispatch (the flags are read from the descriptor the
// dispatch is about to consult) but speak AFTER it, so nothing the dispatch
// itself says can cut this off. Personal entries only: the target rows are a
// different engine function with its own refusal path, still unexamined.
//
// The engine's sentence is the good answer — it names the actual reason
// ("no effect on this target", "not enough charges"). The generic line is the
// fallback for a refusal that carries no reason code, because "nothing
// happened" is exactly what this exists to stop.
uint32_t PendingRefusalStrRef(void* mi, const Cat& cur, int sel,
                              bool* outRefused) {
    *outRefused = false;
    if (cur.kind != CatKind::Personal) return 0;
    uint32_t strref = 0;
    *outRefused = acc::engine_actionbar::VariantRefusal(mi, cur.slot, sel,
                                                        &strref);
    return strref;
}

// interrupt=false throughout: the bare-key path speaks the entry's label
// immediately before firing, and "Medikit" followed by the reason is the whole
// sentence the user needs — cutting the label off to deliver the reason would
// answer a question they can no longer hear. Nothing else speaks on a refusal
// (there is no AddAction, and the out-of-combat close is silent by design), so
// there is nothing for this to queue behind.
void SpeakRefusal(uint32_t strref) {
    // We are about to explain this press ourselves. The no-op watch would
    // otherwise reach the same verdict 350 ms later and say it again, less
    // usefully — the engine's own reason beats the generic line.
    acc::combat::queue::CancelNoOpWatch();
    char text[256] = "";
    if (strref != 0 && acc::engine::LookupTlk(strref, text, sizeof(text)) &&
        text[0]) {
        acclog::Write("UnifiedMenu", "refused — speaking engine reason "
                      "strref=0x%x [%s]", strref, text);
        acc::speech::Speak(text, /*interrupt=*/false);
        return;
    }
    acclog::Write("UnifiedMenu",
                  "refused — no usable engine reason (strref=0x%x); generic "
                  "line", strref);
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ActionRefused),
                 /*interrupt=*/false);
}

// Force the engine's APPEND path. Both DoPersonalAction and
// DoTargetAction wipe the leader's action queue before dispatching
// unless its combat-mode bit (field200_0x440 bit 0) is set — that
// bit is the native Shift-held "queue" flag. Our synthetic dispatch
// bypasses the engine's shift capture, so without it every Enter
// overwrites the previous queued action (Force Valor then Cure left
// only Cure). Set for the dispatch, restored afterwards so the
// creature's real combat mode is untouched.
bool DispatchWithQueueAppend(void* tam, void* mi, const Cat& cur) {
    int prevQueueBit = acc::engine::SetLeaderQueueModeBit(1);

    acc::combat_diag::LogPreFire("menu-enter");
    // Attribute the AddAction this dispatch triggers to the user so its
    // detour speaks the "X, position N" cue (see ArmUserQueueAdd).
    acc::combat::queue::ArmUserQueueAdd();
    // Personal only — a target row acting without a queue add is normal, not a
    // no-op (see ArmNoOpWatch).
    if (cur.kind == CatKind::Personal) acc::combat::queue::ArmNoOpWatch();
    bool ok = Dispatch(tam, mi, cur);
    acc::combat_diag::LogPostFire("menu-enter");

    if (prevQueueBit >= 0) acc::engine::SetLeaderQueueModeBit(prevQueueBit);
    return ok;
}

}  // namespace

// What happens to the menu after a successful fire.
//
// Out of combat, the pause state picks the interaction model:
//
//   World PAUSED (the player pressed the pause key, or the
//   Action Menu auto-pause option froze the world on open) →
//   STACK MODE: queue this action and stay armed so several
//   actions can be lined up without re-opening the menu between
//   each, exactly like the in-combat menu. The world stays
//   paused; Esc (or a manual unpause) commits the queue and
//   closes. The "<action>, position N" cue from the AddAction hook
//   is the confirmation that the menu stayed open on this entry.
//
//   World RUNNING → fire-and-close, matching the sighted mouse
//   radial (click an action → it runs → the radial closes).
//   Keeping the menu open here just adds an Esc step vanilla
//   never charges, and a lingering live surface has misfired.
//   ForceDisarm is the same close
//   the Esc path runs, and we are in the sanctioned
//   input-dispatch context (HandleInputEvent), so it obeys the
//   HARD RULE (no populate off the poll/Open path).
//
// Combat is the encounter-level truth (IsPartyInCombat), not the
// controlled-leader bit, so Tabbing to a not-yet-engaged member
// mid-fight can't collapse the menu into fire-and-close and
// unpause an active encounter — the confusion that shaped the
// party-in-combat auto-close in combat.cpp.
void ApplyPostFireClosePolicy() {
    if (!acc::combat::IsPartyInCombat()) {
        if (acc::engine::WorldIsPaused()) {
            acclog::Write("UnifiedMenu",
                "out-of-combat fire while paused — staying open "
                "(stack mode); announce via AddAction hook");
            return;
        }
        // Close is silent by design: the action's confirmation was just
        // spoken by the AddAction hook ("<action>, position 1" —
        // out-of-combat actions still route through the leader's combat
        // round, and ArmUserQueueAdd above put us in its attribution
        // window).
        acclog::Write("UnifiedMenu",
            "out-of-combat fire (world running) — closing "
            "(fire-and-close)");
        ForceDisarm("fire-out-of-combat");
        return;
    }

    // In combat: stay armed + paused after firing so the user can stack
    // several actions into the engine queue (grenade → force power →
    // attack) without re-pausing between each. The world only resumes —
    // and the queue runs — on Esc (ForceDisarm → EndOverlayPause). The
    // confirmation message ("X, position N") is the cue that the menu
    // is still open on the same entry; press Enter again to re-queue,
    // or arrow to another category. Selection/category are preserved;
    // the next keypress's BuildCategoryList + LocateCat re-locates on
    // the same slot.
}

// Follow-cycling contract: Enter right after cycling fires
// immediately when the user's selected action exists on the new
// target (selection carried by action_id above). When it does
// NOT — the action is unavailable there, or the whole category
// vanished — firing would dispatch something the user never
// heard. Consume this press as orientation instead: announce
// the new target's menu context; the next Enter fires what was
// just spoken.
bool HandleEnter(void* tam, void* mi, const Cat& cur, int sel,
                 bool targetChanged, bool selectionCarried) {
    if (targetChanged && !selectionCarried) {
        char prefix[160] = "";
        std::snprintf(prefix, sizeof(prefix),
                      acc::strings::Get(
                          acc::strings::Id::FmtInteractRadial),
                      g.targetName);
        acclog::Write("UnifiedMenu", "ENTER after follow-cycle — "
            "selection not carried; announcing instead of firing");
        SpeakCategory(tam, mi, prefix);
        return true;
    }

    ApplySelection(tam, mi, cur, sel);
    char label[128] = "";
    ReadLabel(tam, mi, cur, sel, label, sizeof(label));

    // K2 medical entries never dispatch through DoPersonalAction (dead end
    // — see SendUseItemRequestK2). Enter opens the party picker; Shift+
    // Enter, or a picker that found no party rows, fires directly at the
    // controlled leader so Enter never dies.
    if (cur.kind == CatKind::Personal &&
        acc::engine_actionbar::EntryIsMedicalK2(mi, cur.slot, sel)) {
        if (!acc::hotkeys::ShiftHeld() &&
            OpenMedPicker(mi, cur, sel, label)) {
            return true;
        }
        (void)FireMedicalDirect(mi, cur, sel,
                                acc::engine::GetClientLeader(), "self");
        return true;
    }

    bool refused = false;
    const uint32_t refusalStrRef = PendingRefusalStrRef(mi, cur, sel, &refused);

    if (cur.kind == CatKind::Personal) {
        acc::engine_actionbar::LogDispatchDiag(mi, cur.slot, sel);
    }

    RestampTargetRowForDispatch(tam, cur);
    bool ok = DispatchWithQueueAppend(tam, mi, cur);

    // The dispatch ran either way — the engine's own no-op-and-beep is left
    // exactly as it was, so a wrong read here costs a spoken line, never an
    // action that should have fired.
    if (refused) SpeakRefusal(refusalStrRef);

    // The queued-action confirmation ("X, position N" / "X, queue
    // full") is spoken by the CSWSCombatRound::AddAction detour
    // (queue::OnEngineActionAdded) that Dispatch() triggers — one
    // authoritative cue per real add, shared with the bare-key path.
    // No separate pre/post snapshot here: it raced the queue drain and
    // double-announced against the hook.
    acclog::Write("UnifiedMenu", "ENTER kind=%s slot=%d idx=%d label=[%s] "
        "ok=%d — queued; announce via AddAction hook",
        cur.kind == CatKind::Target ? "target" : "personal",
        cur.slot, sel, label, ok ? 1 : 0);

    ApplyPostFireClosePolicy();
    return true;
}

// Fire a personal column's selected entry outright, with no menu.
//
// This is what bare 8 and 9 do for KOTOR 2's combat-behaviour and forms
// columns, and it is deliberately NOT what bare 4..7 do: those keys are the
// ENGINE's, and the mod only announces what the engine fired. Keys 8 and 9
// have no engine action-bar binding in either game (both bind 1..9 solely as
// dialogue-reply keys), so nothing fires unless we do it.
//
// On the announcement: speak the label BEFORE dispatching. If the entry turns
// out to be a real combat-round action, the CSWSCombatRound::AddAction detour
// speaks the authoritative "X, position N" with interrupt=true and simply
// replaces this line — which is the behaviour bare 4..7 already have. If it is
// a stance toggle that never reaches AddAction, this line is the only
// confirmation there is. Correct either way, without the mod having to know
// which kind of entry it just fired.
bool FirePersonal(int col) {
    if (col < 0 || col >= kColumnCount) return false;
    if (ForegroundPanelBlocks()) return false;

    void* mi = acc::engine_actionbar::ResolveMainInterface();
    if (!mi) {
        acclog::Write("UnifiedMenu", "FirePersonal col=%d — main interface "
            "unresolved", col);
        return false;
    }

    Cat c{CatKind::Personal, col};
    const int count = acc::engine_actionbar::VariantCount(mi, col);
    if (count <= 0) {
        char msg[160];
        FormatEmptyColumn(col, msg, sizeof(msg));
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("UnifiedMenu", "FirePersonal col=%d empty -> [%s]",
                      col, msg);
        return false;
    }

    // The shadow is the entry the menu last left the user on, kept in lock-step
    // with the engine's own per-column selection — so this fires what Shift+8
    // / Shift+9 would have shown as current.
    int& sel = ShadowFor(c);
    sel = ClampInt(sel, 0, count - 1);
    ApplySelection(/*tam=*/nullptr, mi, c, sel);

    char label[128] = "";
    ReadLabel(/*tam=*/nullptr, mi, c, sel, label, sizeof(label));
    if (label[0]) acc::speech::Speak(label, /*interrupt=*/true);

    bool refused = false;
    const uint32_t refusalStrRef = PendingRefusalStrRef(mi, c, sel, &refused);

    const bool ok = DispatchWithQueueAppend(/*tam=*/nullptr, mi, c);
    if (refused) SpeakRefusal(refusalStrRef);
    acclog::Write("UnifiedMenu", "FirePersonal col=%d idx=%d/%d label=[%s] ok=%d",
                  col, sel, count, label, ok ? 1 : 0);
    return ok;
}

}  // namespace acc::unified_menu
