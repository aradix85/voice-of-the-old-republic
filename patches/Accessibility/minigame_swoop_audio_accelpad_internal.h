// Internal to the accelerator-pad cues (minigame_swoop_audio_accelpad*.cpp); no
// other file includes it. Here: what those files share.

#pragma once

#include "minigame_swoop_audio_internal.h"

namespace acc::swoop_race {

// Gains sized for the vanilla 5u hit radius: the magnet pulls a bike that reached
// ~8u down to <=5u (about 2.6 u/tick at 8u), an effective window of ~8u.
constexpr float kSwoopMagnetGainFar   = 0.15f;  // pull at the engage edge
constexpr float kSwoopMagnetGainNear  = 0.50f;  // pull dead-on the lane (turret-level)
// Per-tick cap. A per-tick lateral trace measured the PLAYER's own bank at
// ~85 u/s peak (~4.7 u/tick at the ~18 Hz swoop tick), p90 ~46-56 u/s. 5.0 sits
// just ABOVE the player's peak so the magnet can ARREST an overshoot against a
// still-held steer key: last-second misses are overshoots (you reach the pad then
// sail past while holding the key), and below 4.7 the magnet loses that tug-of-war. Contained to near-pad
// overshoot (engages only within the gap; brake only counter-pulls on inward
// velocity), so the player keeps full control elsewhere.
constexpr float kSwoopMagnetMaxStepU  = 5.0f;   // per-tick cap (lane-units)
// Velocity brake (PD derivative): the magnet aims at (padErr − brakeTicks·vel),
// not raw padErr, so when you're closing fast it eases / counter-pulls BEFORE you
// reach the pad — bleeding the overshoot momentum raw position pull can't. Small
// lead (1.5 ticks): enough to anticipate overshoot, not so much you undershoot.
constexpr float kSwoopMagnetBrakeTicks = 1.5f;
// Hold-through-crossing window: within this many seconds of reaching a
// gate's forward position, the pull ramps to fully sticky regardless of lateral
// error, so anticipatory drift toward the next (often opposite-lane) gate can't
// peel the bike off this gate before it scores. Seconds (not units) so the
// window is gear-independent. ~0.35 s ≈ the last ~6 ticks before a crossing.
constexpr float kSwoopMagnetHoldSec   = 0.35f;
// During the pre-race countdown the engine force-centers the bike (speed=0, our
// offset write is clobbered back to lane center), so the magnet would write
// uselessly and spam the heartbeat log. Gate it on the bike actually moving.
// Live: start hold reads speed=0; gear 1 launches to ~9 then 30+ u/s.
constexpr float kSwoopMinRaceSpeed    = 1.0f;   // u/s; below this the race hasn't launched

// ============================================================================
// Accelerator-pad STEERING GUIDE — continuous panned tone.
//
// A single looping tone (acc_boost) panned to the NEXT gate's lateral offset, so
// you "drive toward the sound": the pan DIRECTION says which way and the pan
// MAGNITUDE says how far to swing (a slight pan = nudge, a hard pan = full-lane
// gate). Loudness rises as the gate nears, so you also hear urgency / how soon.
// This is the centering paradigm real racing-game a11y mods use (SpaghettiKart /
// "Super Blind Kart"). We pan our OWN tone, not the bike hum — that hum is not a
// reachable engine source (the swoop Player's <Engine> resref is empty in every
// area GFF, so player+0x144 is null; see the constants block above).
//
// Decoupled pan: the cue keeps the pad's true lateral (X) + vertical (Z) but
// pins forward (Y) to a FIXED depth (kSwoopGuidePanForwardRefM), so pan encodes
// lane offset ONLY and is stable — it doesn't lurch as the gate nears.
//
// Rest zone = ~the catch radius (kSwoopGuideSilenceU): the tone goes silent only
// when you're essentially ON the pad, where the magnet finishes the last units.
// It stays present for all real steering (including small nudges) so you never
// "listen for nothing"; it quiets only when you're locked on. Division of labour:
// you do the coarse positioning by ear, the magnet closes the final gap. So the
// player never has to null the pan precisely on the bike (a human-in-the-loop
// that oscillates); coarse magnitude is what pan gives for free.
//
// Pad world positions come from each CSWMiniEnemy's first model via vtable[+0x64]
// (ReadFollowerPosition); pads are reached via the AsEnemy (vtable[0x1c])
// downcast.
// ============================================================================
// One tick of the pad cues: the readings, the nearest pad ahead, and the line
// the guide tone and the magnet steer to.
struct PadTick {
    Vector    listener = {0.0f, 0.0f, 0.0f};
    void*     player = nullptr;
    Vector    bikePos = {0.0f, 0.0f, 0.0f};
    bool      bikeOk = false;
    Vector    tunnel = {0.0f, 0.0f, 0.0f};  // the bike's offset Vector (+0x1c4)
    bool      tunOk = false;
    float     refY = 0.0f;
    float     bikeSpeed = 0.0f;
    float     playerRadius = 0.0f;
    ULONGLONG now = 0;
    int       slotsSeen = 0;
    int       padsFound = 0;
    int       padsAhead = 0;
    int       minesFound = 0;
    int       aheadSlot = -1;
    Vector    aheadPos = {0.0f, 0.0f, 0.0f};
    float     aheadRadius = 0.0f;  // all pads share one sphere_radius
    bool      virtualTarget = false;
    bool      haveTarget = false;
    // padErr and fwdGap stay the true nearest-pad values (the crossing score
    // relies on them); the detours move only guidePos and guideErr.
    float     padErr = 0.0f;
    float     fwdGap = -1.0f;
    Vector    guidePos = {0.0f, 0.0f, 0.0f};
    float     guideErr = 0.0f;
    int       barDetourSlot = -1;
    float     barY = 0.0f;
    int       mineDetourSlot = -1;
    bool      inGear = false;
};

// Helpers shared across the files, by the file that defines them.

// minigame_swoop_audio_accelpad_detour.cpp
void ApplyBarDetour(void* miniGame, PadTick& t);
void ApplyMineDetour(PadTick& t);

// minigame_swoop_audio_accelpad_assist.cpp
void ApplyMagnet(PadTick& t);
void PredictWallCue(const PadTick& t);
void UpdateWallLoop(const PadTick& t);

}  // namespace acc::swoop_race
