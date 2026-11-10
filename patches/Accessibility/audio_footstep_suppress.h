// Footstep suppression while the leader is stuck. Footsteps mean progress:
// when the walk animation runs but the windowed net speed says the leader
// is grinding an obstacle (StuckWindow in stuck_detect.h), the PlayFootstep
// hook skips the sound and the droid drive-loop hook stops the loop, so the
// silence tells a blind player "you are blocked" without a new cue. Combat
// bypasses both. Tick also drives the free-directions probe
// (audio_stuck_probe.h).
//
// CRITICAL: on both games the PlayFootstep hook sits mid-function at the
// engine's own early-out branch (K1 JZ, K2 CMP+JNZ, with INVERTED return
// polarity; see the handler in the .cpp). Cleaner-looking K1 entry-point
// cuts silenced every footstep because the cut bytes landed across
// TEST EAX,EAX and clobbered ZF before the engine's JZ. hooks.toml and
// kotor2.hooks.toml hold the detail; do not "simplify" without re-reading.

#pragma once

#include <cstdint>

namespace acc::audio::footstep_suppress {

// Per frame from core_tick: samples the leader's position.
void Tick();

// GetTickCount64() at the last leader footstep, 0 if none yet. The
// dead-keyboard watchdog reads it as a liveness channel: footsteps keep
// firing while the player walks into a wall, where position and camera yaw
// both stay put although the keyboard is fine.
uint64_t LastLeaderFootstepMs();

}  // namespace acc::audio::footstep_suppress
