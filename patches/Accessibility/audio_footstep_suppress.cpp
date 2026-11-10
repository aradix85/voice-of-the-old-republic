// Footstep suppression while the leader is stuck — see audio_footstep_suppress.h.

#include "audio_footstep_suppress.h"

#include <windows.h>
#include <cstdint>

#include "audio_stuck_probe.h"
#include "combat.h"          // IsCombatActive: combat bypasses suppression
#include "engine_game.h"
#include "engine_keymap.h"   // ForwardBackwardKeyHeld: the drive-loop push gate
#include "engine_player.h"
#include "engine_reads.h"
#include "log.h"
#include "stuck_detect.h"

namespace acc::audio::footstep_suppress {

namespace {

// Leader footsteps arrive every ~350-600 ms at normal gait and in ~2 s gaps
// when the engine stalls the walk animation against a wall. 1200 ms covers
// the normal cadence with margin, so `walking` holds between steps; a hard
// stall re-arms within one step once the animation resumes.
constexpr uint64_t kFootstepFreshnessMs = 1200;

StuckWindow g_window;
uint64_t    g_last_leader_footstep_ms = 0;

bool IsWalking(uint64_t now_ms) {
    return g_last_leader_footstep_ms != 0 &&
           (now_ms - g_last_leader_footstep_ms) < kFootstepFreshnessMs;
}

// Stuck held without a break for at least min_ms.
bool StuckSustainedFor(unsigned int min_ms) {
    return g_window.Stuck() && g_window.StuckSinceMs() != 0 &&
           GetTickCount64() - g_window.StuckSinceMs() >= min_ms;
}

}  // namespace

uint64_t LastLeaderFootstepMs() { return g_last_leader_footstep_ms; }

void Tick() {
    Vector pos;
    if (!acc::engine::GetPlayerPosition(pos)) {
        // No player: the next one seeds fresh.
        g_window.Reset();
        g_last_leader_footstep_ms = 0;
        stuck_probe::Reset();
        return;
    }
    const uint64_t now_ms = GetTickCount64();
    const StuckWindow::Step step = g_window.Push(pos, now_ms);
    if (step == StuckWindow::Step::Seeded) {
        acclog::Write("FootstepSup", "Tick seed pos=(%.3f,%.3f,%.3f)",
                      pos.x, pos.y, pos.z);
        return;
    }
    if (step == StuckWindow::Step::Skipped) return;

    acclog::Edge("FootstepSup.stuck", g_window.Stuck() ? 1 : 0,
                 "Tick pos=(%.3f,%.3f) net=%.3fm span_ms=%llu wspeed=%.3f m/s stuck=%d",
                 pos.x, pos.y, g_window.NetMetres(),
                 static_cast<unsigned long long>(g_window.SpanMs()), g_window.Speed(),
                 g_window.Stuck() ? 1 : 0);
    stuck_probe::Tick(pos, now_ms, IsWalking(now_ms));
}

}  // namespace acc::audio::footstep_suppress

namespace fs = acc::audio::footstep_suppress;

// CSWCCreature::PlayFootstep detour, both games. The hook replaces the
// engine's own early-out branch on [this+0x20] (skip_original_bytes on both
// entries), so the handler owns the play-or-skip decision, but the branch
// SENSE is inverted between the binaries and the verdict maps to opposite
// return values:
//
// KOTOR 1, at the JZ 0x0061a31a (jumps on field == 0 to the early-out):
//   return 1 -> consumed_exit 0x0061a632, the early-out destructor cascade
//              (footstep suppressed)
//   return 0 -> natural resume 0x0061a320 (audio plays)
//   ESI holds `this` at the cut.
//
// KOTOR 2, at the CMP+JNZ 0x00765eea (jumps on field != 0 to the play path
// and FALLS THROUGH into the early-out cascade):
//   return 1 -> consumed_exit 0x00765f10, the play path
//   return 0 -> natural resume 0x00765ef0, the early-out cascade (suppressed)
//   EAX holds `this` at the cut.
//
// Both games test the same [this+0x20] field (K2 witnessed at 0x00765ee4).
// hooks.toml documents why every "obvious" K1 cut clobbered EFLAGS or EAX;
// kotor2.hooks.toml documents the K2 cut and its inverted polarity.
//
// The verdict covers every creature, not just the leader: KOTOR's footstep
// audio attenuates with distance, so any other step we would hear is close
// enough to fill the silence the player listens for. Combat bypasses it:
// melee positioning nets little progress, and those steps carry situational
// awareness (a second enemy running up).
extern "C" int __cdecl OnPlayFootstep(void* creature) {
    constexpr size_t kPlayGateOffset = 0x20;  // the field the early-out tests
    const int kSuppress = acc::game::IsKotor2() ? 0 : 1;
    const int kPlay     = acc::game::IsKotor2() ? 1 : 0;
    uint32_t gate = 0;
    if (!creature || !acc::engine::TryReadU32(creature, kPlayGateOffset, &gate)) {
        return kSuppress;  // bad pointer: skip the audio safely
    }
    if (gate == 0) return kSuppress;  // the engine's own early-out

    void* leader = acc::engine::GetClientLeader();
    const bool is_leader = leader && creature == leader;
    const bool stuck = fs::g_window.Stuck();
    const bool in_combat = acc::combat::IsCombatActive();
    const bool suppress = stuck && !in_combat;
    if (is_leader) fs::g_last_leader_footstep_ms = GetTickCount64();

    acclog::Write("FootstepSup", "PlayFootstep this=%p leader=%p field20=0x%x "
                  "is_leader=%d stuck=%d combat=%d suppress=%d",
                  creature, leader, gate, is_leader ? 1 : 0, stuck ? 1 : 0,
                  in_combat ? 1 : 0, suppress ? 1 : 0);
    return suppress ? kSuppress : kPlay;
}

// CSWCCreature::UpdateRollingFootstepSound detour, both games. Wheeled and
// floating droids (footstepsounds.2da rows 8/9: T3-M4, astromechs, remotes)
// carry a LOOPING drive sound that this per-frame updater starts and stops
// from the current animation alone: walk/run keeps it playing, anything
// else falls through to an idle branch that stops the source. Grinding a
// wall keeps the walk animation alive, so the loop would play on while the
// droid gets nowhere; it is the droid's dominant sound, its one-shot
// footsteps fire only about once a second.
//
// On a suppress verdict the hook consumes into the engine's OWN idle/stop
// branch, which does its own checks, the Stop and the state clear; the
// untouched updater restarts the loop one frame after movement recovers.
// The handler makes no engine call and reads no creature field.
//
// The verdict is stricter than the one-shot one:
//   - Sustained stuck (kLoopStuckSustainMs): the window dips into stuck for
//     moments during course corrections at droid pace. A skipped click is
//     inaudible there; a cut loop sounds broken.
//   - A forward/backward key must be held to ARM it: pivoting on the spot
//     nets no displacement, and its servo sound is real droid audio the
//     player turns by.
//   - The key does NOT disarm it: once armed, suppression holds until the
//     stuck state clears, or the loop would blip back while the walk
//     animation still runs after the key comes up.
//   - Combat bypass, as for footsteps.
// Gap: a walk the ENGINE drives never arms (no key down); the walk-to
// watchdog announces that case. Like the footstep verdict it is global,
// leader-derived.
//
// KOTOR 1: cut at 0x00610843, the first instruction after the prologue
// parks `this` in ESI (MOV EAX,[ESI+0x430], 6 bytes, replayed on the natural
// path):
//   return 1 -> consumed_exit 0x00610885, the idle/stop branch. EAX and ECX
//              are written there before any read, so the clobber is harmless.
//   return 0 -> natural resume, normal animation dispatch.
// KOTOR 2: the twin cut is documented in kotor2.hooks.toml.
namespace {
constexpr unsigned int kLoopStuckSustainMs = 1200;
bool g_loop_latched = false;  // armed by a push, held until stuck clears
}  // namespace

extern "C" int __cdecl OnUpdateRollingFootstep(void* creature) {
    if (!creature) return 0;
    const bool sustained = fs::StuckSustainedFor(kLoopStuckSustainMs);
    const bool in_combat = acc::combat::IsCombatActive();
    const bool pushing   = acc::engine_keymap::ForwardBackwardKeyHeld();
    if (!sustained || in_combat) {
        g_loop_latched = false;
    } else if (pushing) {
        g_loop_latched = true;
    }
    const int suppress = g_loop_latched ? 1 : 0;
    acclog::Edge("FootstepSup.rolling", suppress,
                 "UpdateRollingFootstep suppress=%d (sustained=%d pushing=%d combat=%d)",
                 suppress, sustained ? 1 : 0, pushing ? 1 : 0, in_combat ? 1 : 0);
    return suppress;
}
