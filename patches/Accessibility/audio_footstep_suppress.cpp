#include "audio_footstep_suppress.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "combat.h"          // IsCombatActive — combat bypasses suppression
#include "engine_keymap.h"   // ForwardBackwardKeyHeld — drive-loop push gate
                             // (keyboard walk keys OR the KOTOR 2 left stick)
#include "engine_offsets.h"
#include "engine_player.h"
#include "audio_stuck_probe.h"
#include "log.h"
#include "engine_game.h"

namespace acc::audio::footstep_suppress {

namespace {

// Windowed net-progress stuck detection (replaced the per-frame 0.3 m/s
// sample 2026-07-15). Wall contact is not a steady state at frame level:
// the engine alternates exact dead-stop frames with micro-slide / bounce
// frames at 1-14 m/s, so any instantaneous threshold flaps (Shyrack cave
// log: 1250 stuck edges in 38 min, ~60% of footsteps leaking during
// wall-grinding). Judge NET displacement over a rolling window instead —
// bounce cancels out, genuine movement accumulates.
//
// KOTOR has exactly two commanded gaits (walk ≈ 2 m/s, run ≈ 5.4 m/s;
// no analog input), so sustained net speed well below walk gait can only
// mean the engine is fighting an obstacle. Hysteresis bands sit under
// the walk gait with a gap against border-speed flapping:
//   enter stuck below 0.8 m/s, exit above 1.2 m/s.
// Semantics: footsteps = progress. A shallow wall-slide at 3-4 m/s keeps
// its steps (you are genuinely covering ground); a steep grind netting
// <1 m/s goes silent.
constexpr uint64_t kWindowMs           = 500;   // rolling judgement window
constexpr uint64_t kMinHistoryMs       = 300;   // no stuck verdict before this
constexpr float    kStuckEnterSpeed    = 0.8f;  // m/s, windowed net speed
constexpr float    kStuckExitSpeed     = 1.2f;  // m/s, windowed net speed

// Below this dt the sample is redundant frame noise; skip it.
constexpr uint64_t kMinDeltaMs = 8;

// Position ring buffer. At the observed ~30-50 ms tick cadence, 500 ms
// spans ~10-16 samples; 32 slots cover slower frames with margin. When
// the window outruns the buffer the oldest surviving sample is used —
// judged over a slightly longer window, which is harmless.
constexpr int kSampleCapacity = 32;
struct PosSample { Vector pos; uint64_t ms; };
PosSample g_samples[kSampleCapacity];
int       g_sample_head  = 0;   // next write slot
int       g_sample_count = 0;
uint64_t  g_last_tick_ms = 0;
bool      g_was_stuck    = false;
uint64_t  g_stuck_since_ms = 0;  // entry time of the current stuck episode

// Freshness window sizing: leader PlayFootstep events arrive every
// ~350-600 ms at normal gait and degrade to ~2 s gaps when the engine
// stalls the walk anim against a wall. The original 200 ms window was
// below even the normal cadence, so `walking` flickered false between
// steps, the progress checkpoint reset every time, and the 2 s
// no-progress window could never complete — 18 sessions of logs
// (through 2026-07-15) show zero probe fires despite 20 s wall-grinding
// episodes. 1200 ms covers the normal cadence with margin; hard-stall
// gaps beyond it still re-arm within one step once the anim resumes.
constexpr uint64_t kFootstepFreshnessMs           = 1200;

uint64_t g_last_leader_footstep_ms = 0;

// Walk-anim freshness: a leader footstep within kFootstepFreshnessMs.
bool IsWalking(uint64_t now_ms) {
    return g_last_leader_footstep_ms != 0 &&
           (now_ms - g_last_leader_footstep_ms) < kFootstepFreshnessMs;
}

}  // namespace

bool WasStuckLastTick() { return g_was_stuck; }

bool StuckSustainedFor(unsigned int min_ms) {
    if (!g_was_stuck || g_stuck_since_ms == 0) return false;
    return (GetTickCount64() - g_stuck_since_ms) >= min_ms;
}

void NoteLeaderFootstep() {
    g_last_leader_footstep_ms = GetTickCount64();
}

uint64_t LastLeaderFootstepMs() { return g_last_leader_footstep_ms; }

void Tick() {
    Vector pos;
    if (!acc::engine::GetPlayerPosition(pos)) {
        // No player loaded — reset so next observation seeds fresh.
        g_sample_count = 0;
        g_sample_head  = 0;
        g_last_tick_ms = 0;
        g_was_stuck = false;
        g_stuck_since_ms = 0;
        g_last_leader_footstep_ms = 0;
        ResetProbeTriggers();
        return;
    }

    const uint64_t now_ms = GetTickCount64();

    if (g_sample_count == 0) {
        g_samples[0] = {pos, now_ms};
        g_sample_head  = 1 % kSampleCapacity;
        g_sample_count = 1;
        g_last_tick_ms = now_ms;
        g_was_stuck = false;
        g_stuck_since_ms = 0;
        acclog::Write("FootstepSup", "Tick seed pos=(%.3f,%.3f,%.3f)",
                      pos.x, pos.y, pos.z);
        return;
    }

    const uint64_t dt_ms = now_ms - g_last_tick_ms;
    if (dt_ms < kMinDeltaMs) return;  // redundant frame — skip the sample
    g_last_tick_ms = now_ms;

    // Push the new sample.
    g_samples[g_sample_head] = {pos, now_ms};
    g_sample_head = (g_sample_head + 1) % kSampleCapacity;
    if (g_sample_count < kSampleCapacity) ++g_sample_count;

    // Reference sample: the NEWEST one at least kWindowMs old, so the
    // judged span stays close to the nominal window instead of growing
    // with buffer depth. Falls back to the oldest sample when none is
    // old enough (early after a seed/reset).
    // Valid samples sit at head-1 (newest, just pushed) back through
    // head-count; the walk starts at newest-1 and must stop AT the oldest —
    // i < count, not i <= count. The <= form read one slot past the oldest,
    // which after a seed/reset is uninitialized or stale from the previous
    // area; its ancient timestamp passes the age test, so the first verdict
    // after every area change was judged against a garbage position
    // (2026-08-04 K2 log: net=95m, span=11 days, stuck latched wrongly).
    const PosSample* ref = nullptr;
    for (int i = 1; i < g_sample_count; ++i) {
        // Walk backwards from newest-1 to oldest.
        int idx = (g_sample_head - 1 - i + 2 * kSampleCapacity)
                  % kSampleCapacity;
        const PosSample& s = g_samples[idx];
        if ((now_ms - s.ms) >= kWindowMs) { ref = &s; break; }
        ref = &s;  // remember the oldest seen so far as fallback
    }

    const uint64_t span_ms = ref ? (now_ms - ref->ms) : 0;
    float netDist = 0.0f;
    float wSpeed  = 0.0f;
    if (ref && span_ms > 0) {
        // Z excluded — vertical jitter on uneven walkmesh isn't motion.
        netDist = DistXY(pos, ref->pos);
        wSpeed  = netDist / (static_cast<float>(span_ms) * 0.001f);
    }

    // Hysteresis: enter stuck only on a full window of evidence, leave
    // on clear net progress, hold state in the dead band between.
    if (g_was_stuck) {
        if (wSpeed > kStuckExitSpeed) {
            g_was_stuck = false;
            g_stuck_since_ms = 0;
        }
    } else {
        if (span_ms >= kMinHistoryMs && wSpeed < kStuckEnterSpeed) {
            g_was_stuck = true;
            g_stuck_since_ms = now_ms;
        }
    }

    // Edge-driven log: full fidelity without per-tick spam.
    acclog::Edge("FootstepSup.stuck", g_was_stuck ? 1 : 0,
        "Tick pos=(%.3f,%.3f) net=%.3fm span_ms=%llu wspeed=%.3f m/s stuck=%d",
        pos.x, pos.y, netDist,
        static_cast<unsigned long long>(span_ms),
        wSpeed,
        g_was_stuck ? 1 : 0);

    const bool walking = IsWalking(now_ms);
    TickStuckAnnounce(pos, now_ms, walking);
    TickCirclingAnnounce(pos, now_ms, walking);
}

}  // namespace acc::audio::footstep_suppress

// CSWCCreature::PlayFootstep detour — BOTH games. The hook replaces the
// engine's own field6_0x20 early-out branch (skip_original_bytes=true on
// both entries) so the handler owns the play-vs-skip decision, but the
// branch SENSE is inverted between the binaries, so the shared verdict
// maps to opposite return values:
//
// KOTOR 1 — hook at the JZ 0x0061a31a (jumps on field==0 to the early-out):
//   return 1 → consumed_exit 0x0061a632, the early-out destructor cascade
//              (footstep suppressed)
//   return 0 → natural resume 0x0061a320 (audio plays)
//   ESI holds `this` at the cut.
//
// KOTOR 2 — hook at the CMP+JNZ 0x00765eea (jumps on field!=0 to the play
// path and FALLS THROUGH into the early-out cascade):
//   return 1 → consumed_exit 0x00765f10, the play path
//   return 0 → natural resume 0x00765ef0, the early-out destructor cascade
//              (footstep suppressed)
//   EAX holds `this` at the cut.
//
// Both games test the SAME [this+0x20] field (K2 witnessed at 0x00765ee4).
// The hooks.toml comment above the K1 entry documents why every "obvious"
// K1 cut location clobbered EFLAGS or EAX; the kotor2.hooks.toml entry
// documents the K2 cut and its inverted polarity.
extern "C" int __cdecl OnPlayFootstep(void* creature) {
    // Per-game verdict encoding — see the polarity note above.
    const int kSuppress = acc::game::IsKotor2() ? 0 : 1;
    const int kPlay     = acc::game::IsKotor2() ? 1 : 0;
    if (!creature) return kSuppress;

    // Mimic the engine's field6_0x20==0 early-out — we replaced its branch.
    uint32_t field20 = 0;
    __try {
        field20 = *reinterpret_cast<uint32_t*>(
            static_cast<unsigned char*>(creature) + 0x20);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return kSuppress;  // bad pointer — abort safely, audio skipped
    }
    if (field20 == 0) return kSuppress;  // engine's natural early-out

    void* leader = acc::engine::GetClientLeader();
    const bool is_leader = (leader && creature == leader);
    const bool stuck = acc::audio::footstep_suppress::WasStuckLastTick();
    // Combat bypass: in-combat movement is legitimately low-net-progress
    // (engine auto-positioning, circling at melee range, shuffle steps),
    // so the windowed detector would read the melee dance as "stuck" and
    // mute steps that carry situational awareness — including a second
    // enemy running up. "Am I blocked" is an exploration question; during
    // combat every footstep passes through.
    const bool in_combat = acc::combat::IsCombatActive();
    // Suppress non-leader footsteps too while the leader is stuck — otherwise
    // a companion or scripted NPC walking nearby fills the silence the player
    // relies on to detect "I'm blocked". KOTOR's footstep audio attenuates
    // with distance, so any non-leader footstep we'd hear is by definition
    // close enough to mask the leader's own silence; safe to mute the whole
    // class until the leader's velocity recovers.
    const bool suppress = (stuck && !in_combat);

    if (is_leader) {
        // Stamps walk-anim freshness for the stuck-direction probe gate.
        acc::audio::footstep_suppress::NoteLeaderFootstep();
    }

    acclog::Write("FootstepSup", "PlayFootstep this=%p leader=%p field20=0x%x "
        "is_leader=%d stuck=%d combat=%d suppress=%d",
        creature, leader, field20,
        is_leader ? 1 : 0, stuck ? 1 : 0, in_combat ? 1 : 0,
        suppress ? 1 : 0);

    return suppress ? kSuppress : kPlay;
}

// CSWCCreature::UpdateRollingFootstepSound detour — BOTH games. Wheeled and
// floating droids (footstepsounds.2da rows 8/9: T3-M4, astromechs, remotes)
// carry a LOOPING drive sound (fs_wheel_drdslow / fs_floatdroid) that this
// per-frame updater starts and stops purely from the current ANIMATION:
// walk/run anims keep the loop playing, any other anim falls through to an
// idle branch that Stops the source and clears the playing state. Grinding
// a wall keeps the walk anim alive, so the loop plays forever while the
// droid gets nowhere — the same gap the windowed stuck detector closes for
// one-shot footsteps, but on the droid's DOMINANT sound channel (its
// PlayFootstep one-shots fire only about once a second and were already
// suppressed, which is why suppression was inaudible on T3: 2026-08-05 K2
// fuel-depot session).
//
// On a suppress verdict the hook consumes into the engine's OWN idle/stop
// branch: the engine performs its own null and playing checks, the Stop,
// and the state clear — and the untouched updater restarts the loop by
// itself one frame after movement recovers. The handler makes no engine
// call and reads no creature field, so there is nothing here to go stale.
//
// The loop's verdict is STRICTER than the one-shot footstep verdict, on
// purpose (first round 2026-08-05 reused the footstep verdict verbatim and
// T3 went silent during ordinary walking):
//   - Sustained stuck (>= kLoopStuckSustainMs), not instantaneous: the
//     windowed detector dips into stuck for sub-second stretches during
//     normal course corrections at droid walking pace. Skipping a single
//     footstep click there is inaudible; cutting a continuous loop reads
//     as broken audio. A wall grind sustains the state for seconds and
//     still goes silent well before the free-directions probe speaks.
//   - A forward/backward key must be held to ARM it: pivoting in place nets
//     zero displacement by definition, and the servo sound while pivoting is
//     legitimate droid audio — the player turns on the spot constantly to
//     navigate by cues, so muting that would be the worse regression. Only
//     "pushing somewhere and getting nowhere" is wall-grinding.
//   - But the key does NOT disarm it: once armed, suppression holds until
//     the stuck state itself clears. Releasing the key at a wall used to
//     unmute instantly while the walk animation still ran, so the loop blipped
//     back for the half second until the engine's idle branch stopped it
//     (2026-08-05 report). The character has not started moving just because
//     the key came up.
//   - Combat bypass as everywhere in this file.
// Known gap: a walk the ENGINE drives (auto-walk into scenery) never arms,
// since no key is down. The walk-to watchdog already announces that case.
// Like the one-shot verdict it is global (leader-derived), not
// per-creature — while the leader is wedged, a nearby drive loop fills
// the silence the player is listening for.
//
// KOTOR 1 — cut at 0x00610843, the first instruction after the prologue
// parks `this` in ESI (MOV EAX,[ESI+0x430], 6 bytes, replayed on the
// natural path):
//   return 1 → consumed_exit 0x00610885, the idle/stop branch. Both EAX
//              and ECX are written there before any read, so the consume
//              path's EAX clobber is harmless.
//   return 0 → natural resume, normal animation dispatch.
// KOTOR 2 — twin cut documented in kotor2.hooks.toml.
namespace {
constexpr unsigned int kLoopStuckSustainMs = 1200;
bool g_loop_latched = false;  // armed by a push, held until stuck clears
}
extern "C" int __cdecl OnUpdateRollingFootstep(void* creature) {
    if (!creature) return 0;
    const bool sustained = acc::audio::footstep_suppress::StuckSustainedFor(
        kLoopStuckSustainMs);
    const bool in_combat = acc::combat::IsCombatActive();
    const bool pushing   = acc::engine_keymap::ForwardBackwardKeyHeld();
    if (!sustained || in_combat) {
        g_loop_latched = false;
    } else if (pushing) {
        g_loop_latched = true;
    }
    const int suppress = g_loop_latched ? 1 : 0;
    acclog::Edge("FootstepSup.rolling", suppress,
                 "UpdateRollingFootstep suppress=%d (sustained=%d pushing=%d "
                 "combat=%d)",
                 suppress, sustained ? 1 : 0, pushing ? 1 : 0,
                 in_combat ? 1 : 0);
    return suppress;
}
