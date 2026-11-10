// KOTOR 2 medical items: the party picker that chooses who gets the medpac or stim.

#include "unified_action_menu_internal.h"

#include "combat_diag.h"        // LogPreFire / LogPostFire around dispatch
#include "combat_queue.h"       // ArmUserQueueAdd — attribute the AddAction
#include "engine_area.h"        // ResolveServerObjectHandle, kInvalidObjectId
#include "engine_input.h"       // kInputNav*, kInputEnter*, kInputEsc*,
                                // kInputHome/End, kInputCatFirst/Last
#include "engine_player.h"      // SetLeaderQueueModeBit (append-vs-replace)
#include "hotkeys.h"            // ShiftHeld
#include "log.h"
#include "strings.h"
#include "speech.h"

namespace acc::unified_menu {

namespace {

// ---- KOTOR 2 medical-item party picker -----------------------------------
//
// Vanilla K2 medical items are two-step: the press arms a "who gets it"
// pick that only a mouse click on a party portrait completes — the wedge
// that killed the whole Medicine row for keyboard play (2026-08-11 flip).
// This submenu IS that second step: Enter on a Medicine-row entry opens
// it, Up/Down clamp through the party members (narrated member
// preselected, else the controlled leader), Enter applies the item,
// Shift+Enter (on the entry or in here) short-circuits to instant
// self-use, Esc backs out to the entry. Bare key 5 never opens it —
// instant self-use stays the one-press combat reflex.

void SpeakMedPickerRow(bool withTitle) {
    if (withTitle) {
        char msg[160];
        std::snprintf(msg, sizeof(msg),
                      acc::strings::Get(acc::strings::Id::FmtMedPickerApplyTo),
                      g.med.names[g.med.sel]);
        acc::speech::Speak(msg, /*interrupt=*/true);
    } else {
        acc::speech::Speak(g.med.names[g.med.sel], /*interrupt=*/true);
    }
}

}  // namespace

// Fire a Medicine-row entry at a specific CLIENT creature via the direct
// use-item request (DoPersonalAction is a dead end for K2 medical items —
// see SendUseItemRequestK2). Arms the same attribution + no-op watch the
// normal dispatch path arms, so the AddAction hook speaks "X, position N" on
// success and the generic line speaks if the server drops the request.
bool FireMedicalDirect(void* mi, const Cat& c, int sel, void* targetCre,
                       const char* how) {
    acc::engine_actionbar::LogDispatchDiag(mi, c.slot, sel);
    uint32_t aid = acc::engine_actionbar::ReadVariantActionId(mi, c.slot, sel);
    if (aid == 0 || !targetCre) {
        acclog::Write("UnifiedMenu",
                      "medical %s fire aborted (aid=0x%08x cre=%p)",
                      how, aid, targetCre);
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::ActionRefused),
                     /*interrupt=*/false);
        return false;
    }
    acc::combat_diag::LogPreFire("med-direct");
    acc::combat::queue::ArmUserQueueAdd();
    acc::combat::queue::ArmNoOpWatch();
    bool ok = acc::engine_actionbar::SendUseItemRequestK2(aid, targetCre);
    acclog::Write("UnifiedMenu", "medical %s fire id=0x%08x sent=%d",
                  how, aid, ok ? 1 : 0);
    ApplyPostFireClosePolicy();
    return ok;
}

// Returns true when the picker opened (the press is consumed). False = no
// party slot resolved; the caller falls through to the plain dispatch
// (instant self-use), so Enter is never dead.
bool OpenMedPicker(void* mi, const Cat& c, int sel, const char* itemLabel) {
    g.med = MedPicker{};
    uint32_t narrated = ResolveNarratedServerHandle();
    uint32_t leaderId = acc::engine::GetObjectHandle(
        acc::engine::GetClientLeader());
    int narratedRow = -1;
    int leaderRow   = -1;
    for (int s = 0; s < 3; ++s) {
        void* cre = acc::engine::GetPartyCreatureBySlotK2(s);
        if (!cre) continue;
        uint32_t id = acc::engine::GetObjectHandle(cre);
        if (id == 0) continue;
        int row = g.med.count;
        g.med.handles[row] = id;
        g.med.slots[row]   = s;
        // Name chain, PC-aware (the PC's display name never resolves by
        // handle — its chargen name lives in the client app, not the
        // creature stats; same layering the Tab announce solved):
        //   1. universal display-name accessor (companions resolve here)
        //   2. server-object name via the engine's own client→server
        //      resolver (the Q/E-cycle path that reads the PC correctly)
        //   3. leader row → GetActiveLeaderName (the shipped Tab solution)
        //   4. chargen-name slot, then the bare slot number as last resort.
        char* nm = g.med.names[row];
        const size_t nmSize = sizeof(g.med.names[row]);
        bool named = acc::engine::GetObjectDisplayNameByHandle(id, nm, nmSize)
                     && nm[0];
        if (!named) {
            void* srv = acc::engine::ClientToServerCreature(cre);
            named = srv && acc::engine::GetObjectName(srv, nm, nmSize) &&
                    nm[0];
        }
        if (!named && leaderId != 0 &&
            ((leaderId ^ id) & ~0x80000000u) == 0) {
            named = acc::engine::GetActiveLeaderName(nm, nmSize) && nm[0];
        }
        if (!named) {
            named = acc::engine::GetPlayerCharacterName(nm, nmSize) && nm[0];
        }
        if (!named) {
            std::snprintf(nm, nmSize, "%d", s + 1);
        }
        if (narrated != 0 &&
            ((narrated ^ id) & ~0x80000000u) == 0) narratedRow = row;
        if (leaderId != 0 &&
            ((leaderId ^ id) & ~0x80000000u) == 0) leaderRow = row;
        g.med.count++;
    }
    if (g.med.count == 0) return false;
    // Preselect: narrated party member beats the controlled leader beats
    // row 0 — "two fast Enter presses" lands on whoever you meant.
    g.med.sel = (narratedRow >= 0) ? narratedRow
              : (leaderRow   >= 0) ? leaderRow : 0;
    g.med.active  = true;
    g.med.itemCat = c;
    g.med.itemSel = sel;
    std::snprintf(g.med.itemLabel, sizeof(g.med.itemLabel), "%s",
                  itemLabel ? itemLabel : "");
    acclog::Write("UnifiedMenu",
                  "medpicker open item=[%s] rows=%d preselect=%d(%s)",
                  g.med.itemLabel, g.med.count, g.med.sel,
                  g.med.names[g.med.sel]);
    SpeakMedPickerRow(/*withTitle=*/true);
    return true;
}

bool HandleMedPickerInput(int code) {
    if (code == kInputEsc1 || code == kInputEsc2) {
        g.med.active = false;
        acclog::Write("UnifiedMenu", "medpicker esc — back to entry");
        if (g.med.itemLabel[0])
            acc::speech::Speak(g.med.itemLabel, /*interrupt=*/true);
        return true;
    }
    switch (code) {
        case kInputNavUp:
        case kInputNavDown: {
            // Clamp at both ends; the repeated name is the edge cue.
            int dir = (code == kInputNavDown) ? +1 : -1;
            g.med.sel = ClampInt(g.med.sel + dir, 0, g.med.count - 1);
            SpeakMedPickerRow(/*withTitle=*/false);
            return true;
        }
        case kInputHome:
        case kInputEnd:
            g.med.sel = (code == kInputHome) ? 0 : g.med.count - 1;
            SpeakMedPickerRow(/*withTitle=*/false);
            return true;
        case kInputEnter1:
        case kInputEnter2: {
            g.med.active = false;
            void* mi = acc::engine_actionbar::ResolveMainInterface();
            if (!mi) return true;
            // Shift+Enter = instant self (the controlled leader); plain
            // Enter re-resolves the focused row through the engine's own
            // party-slot accessor — the PC's pseudo-handle won't resolve
            // through the object array (the "cannot aim at the PC" round),
            // but its slot always does. Handle resolve stays as fallback.
            void* cre = nullptr;
            if (acc::hotkeys::ShiftHeld()) {
                cre = acc::engine::GetClientLeader();
            } else {
                cre = acc::engine::GetPartyCreatureBySlotK2(
                    g.med.slots[g.med.sel]);
                if (!cre) {
                    cre = acc::engine::ResolveClientObject(
                        g.med.handles[g.med.sel]);
                }
            }
            (void)FireMedicalDirect(mi, g.med.itemCat, g.med.itemSel, cre,
                                    "picker");
            return true;
        }
        default:
            // Modal: swallow everything else so the menu underneath can't
            // shift categories while the picker is up.
            return true;
    }
}

}  // namespace acc::unified_menu
