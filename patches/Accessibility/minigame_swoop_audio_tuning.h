// The swoop race as the audio cues see it: the engine layout of the minigame
// objects, the hazard classes, and every tuning constant of the cues. Included
// through minigame_swoop_audio_internal.h.

#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>

#include "audio_cues.h"       // NavCue, GetNavCueResref
#include "engine_offsets_select.h"
#include "engine_offsets_types.h"  // Vector

namespace acc::swoop_race {



// ----- Player and lane offsets -----
//
// Engine confirmed (docs/llm-docs/swoop-accelpad-hit-model.md): hit band
// is player.Sphere_Radius + pad.Sphere_Radius (Taris 6.0, Tatooine /
// Manaan 5.0) in world units; world-X tracks tunnel-X 1:1 (lane ±20).
// CSWMiniGame.player at +0x24; CSWMiniPlayer.offset (tunnel lane coord,
// x = lateral) at +0x1c4.

// ----- KOTOR 2 hazard geometry (bounding-box classification) -----
//
// K2's tracks are not rock fields. The offline atlas of the three tracks'
// obstacle models (docs/llm-docs/swoop-k2-controls-and-jump.md section 7) and
// the first K2 round showed three kinds of "obstacle":
//   * lane-edge POSTS — 90 of Citadel's 105 objects, one pair every 100 units,
//     origins at x=-30/+28 with a 16-unit-tall "hitbump" mesh at |x| 18..25.
//     Only touchable when hugging a wall; sounding them was the "enormous
//     amount of obstacles" the player heard, all noise.
//   * low BARS — 6 units tall, spanning half or all of the lane, whose mesh
//     sits up to 46 units AWAY from the object's origin (MGC01's origin is at
//     y=928, its bar at y=880). Position-based cues put them in the wrong
//     place; the bike must JUMP them (apex ~9.8 > 6).
//   * BLOCKS — anything taller than a jump (Nar Shaddaa's centre posts,
//     Onderon's walls): steer around, like K1's rocks.
//   * SCENERY — the `<track>_MGE*` hangar / shuttle dressing (six on Citadel).
//     Their origins sit ~1000 units below the track and their hitbump box,
//     folded into world space, lands mid-lane at y 798 / 829 / 1294 / 2460
//     / 2460 / 6002 — where nothing collides: round 4
//     drove through the "full-lane block" at y 1294 on the ground with no
//     hit, while the warn loop played for it and for the two "blocks" just
//     before the first bar. Those phantom loops were what made the bars sound
//     like avoid-obstacles. Skipped by name, like the posts.
// So on KOTOR 2 every obstacle is classified once from its collision box
// (Gob part "hitbump", the very part K1's DoObstacleHitCheck tests; whole-gob
// box as the fallback) in world space = origin + local box. Boxes that lie
// entirely beyond |x| >= kLaneEdgeSkipU are posts and are skipped; height <=
// kJumpableHeightU is a bar; everything else a block. K1 keeps its tested
// position-based path (its rocks are their own origin).
constexpr float kLaneHalfWidthU     = 20.0f;  // tunnel clamp (SWMG_SetPlayerTunnelPos x)
constexpr float kLaneEdgeSkipU      = 17.0f;  // box wholly beyond this = edge post
constexpr float kJumpableHeightU    = 8.0f;   // jump apex ~9.8 (v0 30, g 45.8)
constexpr float kBarLaneMarginU     = 2.5f;   // bike sphere radius 2 + slack
// Speed units → world units per second. The follower's speed field is not
// world u/s: the bike covers 0.665 world units per second per speed unit,
// measured over 56 one-second windows at speeds 30–250 in round 12
// (0.665–0.668 in every speed bucket). Every time-to-contact below is in
// REAL seconds through this factor.
constexpr float kSpeedToWorldPerSec = 0.665f;
// Jump physics (onjump.ncs: SWMG_SetJumpSpeed(30), gravity 45.8): the arc
// z(t) = 30t - 22.9t^2 lands after 1.31 s and is above a 6.3-unit bar for
// t in [kJumpOverBarFromSec, kJumpOverBarUntilSec] after the press. Round 12
// logged 21 flights of 1.297–1.328 s at every speed.
constexpr float kJumpGravity          = 45.8f;
constexpr float kJumpOverBarFromSec   = 0.26f;
constexpr float kJumpOverBarUntilSec  = 1.05f;
constexpr float kAirborneZ            = 0.25f;  // offset.z above this = in the air
// The cue means "press NOW": it fires kJumpCueLeadSec before the bar, so the
// press window (0.26–1.05 s before the bar) opens 0.55 s after the beep and
// closes 1.34 s after it. Round 12's presses came 0.75–1.27 s after the beep
// (five bars); at a 1.5 s lead one of them fell past the window's end (MGC03,
// landed short of MGC04). A 2.0 s lead was too early: the player pressed
// within 0.1 s of the beep and landed just short of the bar.
// The jump itself is the player's: the mod never presses it (the player's
// call: cues inform, they do not play). If the bike is airborne from an early press and touches down
// with the bar still inside the window, the beep repeats once ("again").
constexpr float kJumpCueLeadSec     = 1.6f;
constexpr float kJumpCueMinLeadSec  = 0.38f;  // too late to help below this
// Bar pairs (Citadel: MGC03/04 188 u apart, MGC05/06 96 u): when ONE jump can
// clear both — the second bar's far edge still inside the over-bar window at
// the current speed, with a press window at least kPairMinWindowSec wide —
// the beep for the first bar is placed so the player's usual reaction
// (kJumpReactionSec, round 12's median) lands mid-window, i.e. LATER than the
// single-bar lead. Same sound; the player learns nothing new. Otherwise the
// normal lead, and the second bar is handled on the ground by the free-side
// guide target (kBarFreeClearU) — a missed pad, not a park. On Citadel the
// pair lead only fires above ~330 speed on the 96 u pair.
constexpr float kJumpReactionSec    = 0.9f;
constexpr float kPairMinWindowSec   = 0.3f;
// Where the beep sits tells you the alternative: a half-bar with an open
// strip (kPostContactU block: posts and mines removed) gets the beep placed
// at the strip's real lateral offset, like the guide tone, so the angle says
// HOW FAR ("jump, or steer that far"); a bar with no strip (full lane, the
// ±13 centre bars, or a strip closed by a mine — MGC06/MGC08) sits in the
// middle ("you must jump"). A fixed hard pan said "left" for a 4 u lean and
// a 12 u swing alike.
constexpr const char* kJumpNowCueResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopJumpNow);

enum HazardClass : uint8_t { kHazardBlock = 0, kHazardBar = 1, kHazardPost = 2,
                             kHazardScenery = 3 };
enum HazardState : uint8_t { kHazardUntried = 0, kHazardOk = 1, kHazardFailed = 2 };
struct HazardInfo {
    uint8_t state;       // HazardState
    uint8_t cls;         // HazardClass (valid when state == kHazardOk)
    uint8_t from_part;   // 1 = "hitbump" part box, 0 = whole-gob box
    uint8_t jump_cued;   // bar: jump-now cues fired for this pass (max 2)
    uint8_t park_cued;   // bar: the parked "you must jump" beep fired this park
    Vector  mn, mx;      // model-local collision box
};
// The sample looped per obstacle: a short metallic grenade bounce with a
// steady pitch, which looped reads as a clicking the ~0.7 s in-range window
// cannot miss (a sustained drone's faint attack is lost in it).
constexpr const char* kObstacleWarnLoopResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopObstacleWarn);

// ----- Decoupled cue: lateral→pan, real distance→loudness -----
//
// The classic audiogame "one looping sound that gets louder and pans"
// works only when the two facts it carries land on SEPARATE, each-
// perceptible channels. With a plain 3D source they don't: pan and
// loudness both fall out of the single position, so forward distance
// (huge) governs both and the left/right offset (small) becomes an
// inaudible ~2° whisper. We split them:
//
//   1. PAN (which lane / how far to steer) ← LATERAL only.
//      The cue position keeps lateral (X) and vertical (Z) true, but
//      replaces the forward (Y) component with a FIXED reference depth
//      (kSwoopPanForwardRefM). So pan = atan(lateral / fixed_ref): it
//      depends ONLY on your lane error, scaled so a quarter-lane offset
//      reads as a clear pan. Crucially the reference is CONSTANT, not the
//      real (or a compressed real) distance — so the pan is stable: it
//      moves only when you steer or the pad's lane differs from yours,
//      never lurching as the pad passes. (A compressed real distance
//      inflates the pan as the target nears, which made players overshoot;
//      the true distance makes far targets a ~2° whisper.)
//
//   2. LOUDNESS (how soon / approach speed) ← REAL distance.
//      Driven by us via LoopSource::UpdateVolume on a linear curve over
//      the real 3D distance (full up close → kSwoopVolFarByte at the
//      300 m edge). Keeps the full distance cue, including speed: the
//      faster you close, the faster it swells. The FLAT BAND below makes
//      the engine apply full volume regardless of the source's (now
//      fixed-depth, hence near) position, so our manual curve is the sole
//      loudness control. Pan is unaffected by the band — only attenuation
//      magnitude is, and we want that flat.
//
//   kSwoopPanForwardRefM — fixed depth the pan is read against. Smaller =
//      stronger pan (a given lane offset reads as a wider angle), AND a
//      steeper gradient near centre — which is the point. 6 m: at 12 m
//      ~10 of 22 pad misses were undershoots, stopping 6-11 units short,
//      because "sounds centred" spanned wider than the ±5 unit catch band
//      (5 units off read only ~24°). At 6 m the same 5-unit error reads
//      ~40° and 1 unit ~9.5° — double the near-centre sensitivity, so the band between "aligned" and "close"
//      is far more audible. Shared by obstacle + accelpad cues so the two
//      stay on one coherent projection (an obstacle and a pad at the same
//      lateral offset pan identically). Trade-off: far targets saturate to
//      hard L/R sooner, which is fine — out there you only need "go right",
//      the fine resolution that matters is near the catch band.
//   kSwoopFlatBandMaxM / kSwoopFlatBandMinM — both past any source's
//      fixed-depth distance, so engine attenuation is flat (full).
//   kSwoopVolNearByte / kSwoopVolFarByte — linear loudness ramp endpoints:
//      near → full, far (dist = cue range) → ~⅓. Raise the far byte if
//      distant pads are too quiet to localise; lower it for more contrast.
constexpr float       kSwoopPanForwardRefM = 6.0f;
constexpr float       kSwoopFlatBandMaxM   = 350.0f;
constexpr float       kSwoopFlatBandMinM   = 400.0f;
constexpr int         kSwoopVolNearByte    = 127;
constexpr int         kSwoopVolFarByte     = 50;
// Pan depth for the accelpad STEERING GUIDE specifically (the obstacle cues keep
// the 6 m shared depth above). Opened to 10 m so the pan angle spreads MAGNITUDE
// across the lane — "how far to swing" — for the big alternating gates: at 10 m a
// 12 u offset reads ~50°, a 35 u offset ~74°, so far gates stay distinguishable
// instead of both saturating to hard-pan. The magnet owns near centre, so the
// guide optimises for far-gate magnitude. (Larger = more linear magnitude, less
// near-centre angle — fine, the deadzone + magnet cover near centre.)
constexpr float       kSwoopGuidePanForwardRefM = 10.0f;
// The guide rests (goes silent) only when you're essentially ON the pad — within
// this lateral error, roughly the catch radius — NOT across the whole assist
// gap: most steering is small nudges, and a tone that rests across the gap falls
// silent in exactly that case ("listening for something and hearing nothing").
constexpr float       kSwoopGuideSilenceU   = 5.0f;
// Volume floor for the guide so distant gates stay clearly audible — the shared
// distance curve ramps down toward ~⅓ at range, which let the tone blend into the
// race mix. Floored here so the guide always cuts through (the cue slider still
// scales it).
constexpr int         kSwoopGuideMinVolByte = 118;  // 90 was too quiet in the K2 mix

// Track-axis probe verdict — see TrackAxisUsable.
enum {
    kAxisUndecided = 0,
    kAxisAccepted  = 1,
    kAxisRejected  = 2,
};
// Engine grounding (CSWMiniPlayer::{AdjustPosition,Control,DoInertia,
// KeepInTunnel}): swoop steering is a BANK LEVEL clamped to [-10,+10]
// (AdjustPosition.field10_0x1d0), NOT a momentum accumulator, and the only
// lateral "inertia" is collision-plane bump inertia (DoInertia, gated on
// CSWMiniGame::use_inertia and only non-zero at a wall). Live traces: lateral
// velocity reverses in ~5-8 ticks and the real post-release coast is ~1 unit.

// ----- Predictive overshoot / wall cue -------------------------
// Fire the existing wall-impact sound ~kWallLeadTicks BEFORE the bike pins,
// instead of on contact, so the player starts re-steering that much sooner (you
// can't out-react the pin, but you waste less time stuck against the wall). Lead
// is small on purpose: at top lateral speed the 40u lane crosses in ~0.3s, so a
// big lead would fire across most of the lane. ~3 ticks ≈ 0.1s. The cue is
// guarded to a genuine OVERSHOOT (moving away from the target pad toward the
// wall) so it doesn't cry wolf on the ~38% of pads that legitimately sit at the
// edges — that guard is one sign comparison, not a new subsystem.
constexpr float       kWallLeadTicks    = 3.0f;   // ticks of velocity look-ahead (~0.1s)
constexpr float       kWallEdgeOffsetU  = 20.0f;  // lane half-width in tunnel-offset units
// KOTOR 2: the wall is not free. Lane-edge posts line both sides (mesh from
// |x| 18.3, bike radius 2 → contact at |x| ≈ 16.3) and a post hit is a full
// obstacle hit — 50 hit points, gear 0, parked against the next post. So on
// K2 the cue has to arrive early enough to STOP steering outward, not just to
// shorten the pin: it fires at the post line, not the clamp, with a ~0.4 s
// lead (12 ticks) instead of 0.1 s. The overshoot guard still applies (only
// when moving away from the target pad), so a deliberate swing toward a
// side-lane pad at 11 stays silent. Round 6 drifted
// from 18 to 20 at speed 155 with no cue at all.
constexpr float       kWallLeadTicksK2  = 35.0f;   // ~1.2 s at 30 ticks/s (20 = 0.3-0.7 s before every post hit of round 11, not enough)
constexpr float       kWallEdgeOffsetK2U = 16.0f;

constexpr float       kWallMinVel       = 0.5f;   // ignore slow drift (u/tick)
constexpr ULONGLONG   kWallCueDebounceMs = 400;   // don't machine-gun while pinned
constexpr float       kWallCuePanM      = 5.0f;   // ±pan of the cue to the wall side
// KOTOR 2 wall LOOP. One thump at a predicted crossing was not enough: round
// 7 took 22 of 26 hits on the posts — six arriving at the wall 0.7 s after a
// thump the player did not register, sixteen relaunching from a park at
// |x| = 20. Player's call: warn earlier and continuously, no rail. So on
// K2 the wall thump is a LOOP on the wall's side that runs while the bike is
// in the outer band (|x| >= kWallBandU — pads sit at 11, the post contact
// line is 16.3) or projected to reach the post line within the lead, and
// while parked against a post in any gear. It stops as soon as the bike is
// back inside (hysteresis kWallBandHystU). K1 keeps the one-shot: its wall is
// free, the cue there only shortens a pin.
constexpr float       kWallBandU        = 12.5f;
// Mine-aware guide line (KOTOR 2). Citadel's 24 mines sit on the pad lanes
// and on the diagonals between pads (126 at x 1.5 / y 320 on the centre
// lane between the first two pads; 141, 142, 145 on the diagonals). The
// player steering by the pad tone follows the straight line to the next pad
// and runs over them: round 9 took four mines in one
// race, 420 of 600 hit points, more than every post and bar together. A
// sighted player sees the mine and bends the line; this bends the tone. When
// a mine lies within kMineClearU of the straight line from the bike to the
// next pad, the guide (tone AND magnet) aims at a point kMineClearU beside
// the mine — on the pad's side, or toward the lane centre if the mine sits
// on the pad lane — clamped inside kMineDetourMaxU to stay out of the wall
// band. Once the mine is behind the bike the target is the pad again. padErr
// itself (crossing scores, diagnostics) is never touched.
constexpr float       kMineClearU       = 7.0f;   // mine 2 + bike 2 + slack
constexpr float       kMineDetourMaxU   = 12.0f;  // never detour into the wall band
constexpr float       kMineBehindU      = 4.0f;   // mine this far behind = passed
// Bar-aware line (KOTOR 2). Every right-blocked half-bar on Citadel has the
// next pad 55 u behind it at x = 11, on the BLOCKED side, so the straight
// line to the pad leads through the bar. Both parks of round 12's best race
// (73 s; a clean run computes to ~37 s) were exactly that: the guide pulled
// the bike from the free side (x -1.6) to x 4 in the 0.3 s before MGC06, and
// from x 4 to 11.6 in the air before a flight that landed 33 u short of MGC04.
// Rule: the bike will be GROUNDED in front of some bar — the nearest bar whose
// far edge is not cleared by the current flight (grounded: every bar ahead;
// airborne: landing point = bike y + remaining flight time × world speed,
// validated on all 21 flights of round 12 to within 1–5 u). If that bar is
// inside the look-ahead, has a free side, and the line to the pad crosses its
// span there, the guide (tone AND magnet, like the mine detour) aims at the
// free side, kBarFreeClearU beyond the bar's end. Airborne with the landing
// past the bar by kBarLandClearU: the pad. The decision is sticky until the
// bar is behind the bike, so a flight never swings back into the span late.
// Worst case at a half-bar becomes a missed pad (~5 % speed), never a park.
constexpr float       kBarFreeClearU    = 5.0f;   // bike 2 + slack beyond the bar's end
constexpr float       kBarLandClearU    = 12.0f;  // landing this far past the far edge = cleared
constexpr float       kBarDetourLeadSec = 2.5f;   // grounded: bars within this much travel
constexpr float       kBarDetourMinLeadU = 60.0f; // ... but never less than this (parked/relaunch)
// Airborne: a bar the landing does not clear counts only if it arrives too
// soon for a second press (reaction + the window's early edge + slack).
constexpr float       kReJumpSec        = kJumpReactionSec + kJumpOverBarFromSec + 0.15f;
// The free side is not free: Citadel puts a mine in the gap beside every
// left-free half-bar but MGC04 (MGC03: x -2.7 twelve units before the bar;
// MGC06: x -7.2 nine units after; MGC08: x -8.8 nineteen after). Round 13
// drove into the MGC03 and MGC06 mines five times in
// three races at the plain "bar end minus 5" target. So each half-bar's
// free side is a STRIP: from the bar's end (kBarFreeClearU) to the post
// contact line (kPostContactU, less slack), minus every mine within
// kBarMineWindowU of the bar, each removed with its lateral reach plus
// slack. The reach is 3-D: the mines are raised spheres (z 44-47 against
// the bike's ~40), and a low mine hit at 3.2 u lateral while a high one was
// passed clean at 3.1 — so reach = sqrt(R² − dz²) with R = mine radius +
// bike radius, both read live (fallback below). The widest surviving piece
// wins if it is at least kBarStripMinU; a wide strip targets its bar-near
// edge, a narrow one its centre. No surviving piece = the bar is a
// must-jump: centred beep, no ground detour. The `mine pass:` log line
// records every close pass (minimum 3-D distance, its dx/dy/dz, hp before
// and after) so the 3-D model can be confirmed or corrected from a log.
constexpr float       kPostContactU     = 16.3f;  // post mesh from 18.3, bike radius 2
constexpr float       kBarStripSlackU   = 1.0f;   // beyond posts / mine reach
constexpr float       kBarStripMinU     = 3.0f;   // narrower = not a path = no free side
constexpr float       kBarStripWideU    = 8.0f;   // at least this: aim at the bar-near edge
constexpr float       kBarMineWindowU   = 40.0f;  // mine within this of the bar's front edge
// Mine collision reach, MEASURED (round 14, `mine pass:` lines): every mine
// reports sphere_radius 2.0 like a pad, but a hit
// was recorded at a 3-D distance of 5.09 u and a clean grounded pass at
// 6.20 u, and three flights 9 u above a mine at 3.3 u lateral were clean —
// so the test is spherical about the mine's origin with a combined reach
// between 5.1 and 6.2, not sphere_radius + sphere_radius. The upper bound
// is used; the live radius is still logged. With it MGC06's corridor between
// its mine and the posts is 1.7 u — not a path — and MGC06 is a must-jump.
constexpr float       kMineCollisionRadiusU = 6.2f;
constexpr float       kMinePassWindowU  = 30.0f;  // forward window of the mine-pass log
// After the last pad nothing aimed the guide, so the mine-aware line did
// not exist there and the centre-lane mine 186 u after Citadel's last pad
// was hit in every race of rounds 12–14 (100 hp, ~1 s). A virtual target
// straight ahead of the bike keeps the mine detour alive; the magnet stays
// off unless a detour is active (a target at the bike's own x must not
// damp the player's steering).
constexpr float       kVirtualTargetAheadU = 400.0f;
// Riding a strip beside the posts (MGC03 at x −12.5) the wall loop's
// projected-crossing trigger chattered on and off six times in 1.5 s from
// drift velocities of 0.06–0.26 u/tick (round 14, 34:12–34:13). While the
// guide asks for the band, the projection needs a real velocity.
constexpr float       kWallLoopBandMinVel = 0.3f;   // u/tick
// Ground rule: a half-bar's strip is targeted on the ground only when a jump
// would not save the bike — the bar is part of a PAIR (another bar within one
// flight plus a re-jump of it, either way), or the bike is parked. An
// isolated half-bar keeps the pad line and the beep, so the pad behind it
// (8 behind MGC02, 29 behind MGC08) stays catchable in the air; a flight
// that lands short still falls back to the strip (the airborne rule). And
// the detour waits for a pad that lies BEFORE the bar unless the pad sits in
// the span too close in front of it to move over afterwards
// (kBarAfterPadLeadSec): round 13 lost pads 7 and 10 in every race to a
// detour that engaged 2.5 s early.
constexpr float       kBarAfterPadLeadSec = 1.0f;
// Parked-in-front-of-a-bar classification (ParkedBar in the header): the
// bar's front edge within this far ahead of the parked bike, and the wall
// threshold the stuck hint already uses (kStuckWallU in the race module).
constexpr float       kParkedBarAheadU  = 15.0f;
constexpr float       kParkedWallU      = 13.0f;
constexpr float       kBarStripRestU    = 1.5f;   // guide rests only this close to a strip target
constexpr float       kWallBandHystU    = 1.5f;
constexpr float       kWallLoopPanM     = 8.0f;   // lateral offset of the loop source
constexpr float       kWallLoopFwdM     = 2.0f;   // slight forward so it reads beside, not behind
constexpr const char* kWallCueResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopWallImpact);
// Fallback combined accel-pad hit radius for the CROSS hit= flag, used only
// when a live sphere_radius read faults (the scoring sums player + pad radius,
// the engine's swept sphere-vs-sphere test). Stock: Taris 6 (player 3 + pad 3),
// Tatooine/Manaan 5. See swoop-accelpad-hit-model.md.
constexpr float       kAccelpadHitRadiusU = 6.0f;

}  // namespace acc::swoop_race
