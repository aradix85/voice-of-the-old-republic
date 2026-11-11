// The selected target: its announcement with range cue, cycling targets,
// and the lead probes that measure where to aim ahead of it.

#include "minigame_turret_internal.h"

#include "audio_bus.h"        // PlayCue3D — one-shot lock-confirmation ping
#include "hotkeys.h"          // Q/E target-cycle bindings
#include "log.h"
#include "speech.h"
#include "strings.h"          // Get(TurretGameStarted/Controls/Ended)

namespace acc::turret_game {

// ----- LeadCheck self-measuring diagnostic -----
// Enqueue this tick's linear & curve intercepts (world frame) tagged with the
// bolt-arrival time; when one matures, compare both to the fighter's ACTUAL
// position then and log the miss distances. Lets us prove/tune the curve model
// from the log alone. Slot-tagged so a probe outlives a Q/E switch harmlessly.
void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs) {
    State::LeadProbe& p = g_state.lead_probes[g_state.lead_probe_next];
    p.dueMs = dueMs; p.slot = slot; p.lin = linW; p.curve = curveW;
    g_state.lead_probe_next = (g_state.lead_probe_next + 1) % State::kLeadProbeCount;
}

void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now) {
    for (int i = 0; i < State::kLeadProbeCount; ++i) {
        State::LeadProbe& p = g_state.lead_probes[i];
        if (p.dueMs == 0 || now < p.dueMs) continue;
        if (p.slot == slot) {
            const float lx = p.lin.x - fighterWorldNow.x, ly = p.lin.y - fighterWorldNow.y, lz = p.lin.z - fighterWorldNow.z;
            const float cx = p.curve.x - fighterWorldNow.x, cy = p.curve.y - fighterWorldNow.y, cz = p.curve.z - fighterWorldNow.z;
            const float el = std::sqrt(lx * lx + ly * ly + lz * lz);
            const float ec = std::sqrt(cx * cx + cy * cy + cz * cz);
            acclog::Write("LeadCheck", "slot=%d linErr=%.1f curveErr=%.1f gain=%.1f",
                          slot, el, ec, el - ec);
        }
        p.dueMs = 0;  // consume (or discard a stale-slot probe)
    }
}

// Play the range-boundary marker (c_drdastro_atk1) at a compressed, always-
// audible point toward the fighter so it pans correctly — the same trick the
// lock locator uses. Returns the position used, so a LEAVE event can schedule
// its second tap at the same spot.
Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener) {
    Vector pos = targetPos;
    if (dist > 0.0f) {
        const float kk = kPegMinDist / dist;
        pos.x = listener.x + (targetPos.x - listener.x) * kk;
        pos.y = listener.y + (targetPos.y - listener.y) * kk;
        pos.z = listener.z + (targetPos.z - listener.z) * kk;
    }
    acc::audio::PlayCue3D(kRangeCueResref, pos);
    return pos;
}

// Lock onto census entry `idx`: set the selection, speak "Fighter N, D
// metres", and fire a LOUD one-shot positional cue toward the fighter so the
// player hears which way to swing — independent of where the aim currently
// points (the continuous peg is faint until the aim closes in). Shared by the
// Q/E cycle and the auto-advance on lock loss.
// `speak` gates the spoken "Fighter N, D metres" line. It is false for the
// silent re-pick after a kill — there the destroyed-fighter announce is the
// only speech, and this just relocks + fires the directional locator ping so
// the player can swing onto the next target without a second utterance.
void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[],
                            const Vector occPos[], const TargetInfo occInfo[],
                            int occCount, const Vector& listener, bool speak) {
    if (idx < 0 || idx >= occCount) return;
    g_state.selected_slot = occSlot[idx];
    g_state.last_selected_hp = -1;  // fresh lock — next tick re-baselines HP
    g_state.have_last_pos    = false;  // and re-baselines velocity
    g_state.have_vel         = false;
    g_state.have_accel       = false;  // and the turn estimate
    g_state.accel_samples    = 0;
    g_state.selected_on_target = false;  // re-arm the "fire now" cue
    g_state.selected_behind    = false;  // re-evaluate the behind-gate cleanly
    const int number = NumberForSlot(occSlot[idx]);

    const int meters = WholeMetres(occDist[idx]);
    if (speak) {
        char buf[128];
        const char* noun = g_state.area_noun[occSlot[idx]];
        if (noun[0]) {
            std::snprintf(buf, sizeof(buf),
                          acc::strings::Get(acc::strings::Id::FmtTurretTargetNamed),
                          noun, number, meters);
        } else {
            std::snprintf(buf, sizeof(buf),
                          acc::strings::Get(acc::strings::Id::FmtTurretTarget),
                          number, meters);
        }
        acc::speech::Speak(buf, /*interrupt=*/true);
    }

    // One-shot confirmation at a compressed (always-audible) position in the
    // fighter's direction, full volume — "locked, it's this way". Plays even
    // on a silent re-pick (it's a locator cue, not speech). Uses the SHORT tick
    // sample, not the ~1 s lock beep: rapid Q/E cycling fires this on every
    // switch, and overlapping one-shots of the long sample drone into "scrilling".
    const float sd = occDist[idx];
    Vector pingPos = occPos[idx];
    if (sd > 0.0f) {
        const float kk = kPegMinDist / sd;
        pingPos.x = listener.x + (occPos[idx].x - listener.x) * kk;
        pingPos.y = listener.y + (occPos[idx].y - listener.y) * kk;
        pingPos.z = listener.z + (occPos[idx].z - listener.z) * kk;
    }
    acc::audio::PlayCue3D(kTickCueResref, pingPos);

    // Baseline the range-edge tracker to this lock's actual state. If it's
    // already inside killable range, fire the in-range (ENTER) cue now — so a
    // Q/E onto a close target (and the opening cluster after the cutscene)
    // announces "you can hit this" immediately, not only on the next crossing.
    const float killable = KillableRangeM(occInfo[idx].radius,
                                          TargetSpeed(&occInfo[idx], 0.0f));
    const bool inRangeNow = (sd > 0.0f && sd <= killable);
    g_state.selected_in_range = inRangeNow ? 1 : 0;
    if (inRangeNow) {
        PlayRangeCueAt(occPos[idx], sd, listener);
        acclog::Write("Turret", "range cue: ENTER (on lock) slot=%d dist=%.0fm",
                      g_state.selected_slot, sd);
    }

    acclog::Write("Turret", "select -> slot=%d number=%d pos=%d/%d dist=%dm speak=%d",
                  g_state.selected_slot, number, idx + 1, occCount, meters, speak ? 1 : 0);
}

// Q/E target cycle. Q steps to the previous (nearer) target, E to the next
// (farther), wrapping at the ends. Polled inside the hotkeys BeginTick/
// EndTick window (core_tick calls turret Tick there), so Pressed() sees a
// clean rising edge.
void HandleTargetCycle(const int occSlot[], const float occDist[],
                       const Vector occPos[], const TargetInfo occInfo[],
                       int occCount, const Vector& listener) {
    const bool next = acc::hotkeys::Pressed(acc::hotkeys::Action::TurretCycleNext);
    const bool prev = acc::hotkeys::Pressed(acc::hotkeys::Action::TurretCyclePrev);
    if (!next && !prev) return;
    // A deliberate pick — autoaim's makeability retarget defers to it for a beat.
    g_state.last_manual_cycle_ms = GetTickCount64();

    if (occCount <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::TurretNoTargets), /*interrupt=*/true);
        g_state.selected_slot = -1;
        acclog::Write("Turret", "cycle: no targets");
        return;
    }

    int idx = CensusIndexOf(occSlot, occCount, g_state.selected_slot);
    if (idx < 0) {
        // No live selection — first press lands on nearest (E) / farthest (Q).
        idx = next ? 0 : occCount - 1;
    } else {
        idx += next ? 1 : -1;
        if (idx < 0)          idx = occCount - 1;  // wrap
        if (idx >= occCount)  idx = 0;
    }
    AnnounceSelectedTarget(idx, occSlot, occDist, occPos, occInfo, occCount, listener);
}

}  // namespace acc::turret_game
