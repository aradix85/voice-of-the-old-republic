// Internal to the minigame_swoop_audio module (minigame_swoop_audio*.cpp); no other file includes
// it. The public interface is minigame_swoop_audio.h. Here: what the files share.

#pragma once

#include "minigame_swoop_audio.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstddef>

#include "audio_bus.h"        // PlayCue3D — co-pilot directional / aligned
                              //     one-shots (discrete steering commands)
#include "audio_cues.h"       // NavCue + GetNavCueResref (centralised
                              //     resref vocabulary — Audio glossary
                              //     and live race fire the same samples)
#include "audio_loop.h"       // LoopSource — sustained warning per
                              //     obstacle / nearest-pad cue
#include "engine_area.h"      // ReadGlobalNumber — MIN_RACE_GEAR gates the magnet
#include "engine_game.h"      // IsKotor2 — obstacle-density concurrency budget
#include "engine_offsets.h"   // Vector
#include "engine_player.h"    // GetCameraPosition (primary listener
                              //     anchor — engine swaps the player
                              //     creature during the race, so
                              //     GetPlayerPosition is fallback)
#include "minigame_aim.h"     // shared offset read/write + magnetism (the
                              //     turret aim-assist port — lateral steering)
#include "log.h"
#include "engine_offsets_select.h"

namespace acc::swoop_race {

// SEH read primitives + minigame-object-array resolution are shared across
// the minigame TUs (see minigame_aim.h). Brought into unqualified scope so
// the dense reads below read as they did when each file had its own copy.
using acc::minigame::SafeReadPtr;
using acc::minigame::SafeReadU32;
using acc::minigame::SafeReadFloat;
using acc::minigame::SafeReadVector;
using acc::minigame::ResolveMgoArray;
using acc::minigame::CallAsCast;
using acc::minigame::ReadFollowerPosition;
using acc::minigame::ReadFollowerModelGob;
using acc::minigame::ReadGobBoundingBox;
using acc::minigame::ReadGobPartBoundingBox;

// ----- Global minigame-object-array layout (Ghidra-confirmed 2026-05-24).
//
// The first pass mistakenly treated CSWMiniGame+0x44 as a pointer to a
// flat obstacle array; live read returned obstacle[1]=0x00000001 and
// crashed on dereference. The correct path comes from decompiling
// CSWVirtualMachineCommands::ExecuteCommandSWMG_GetFollowerPosition
// @0x5cc280 + CSWMiniGameObjectArray::GetMiniGameObject @0x66bf30:
//
//   AppManager (*0x7a39fc) +0x4
//     -> CClientExoApp +0x4
//       -> CClientExoAppInternal +0x0
//         -> CSWMiniGameObjectArray (255 ptr slots, handle-indexed)
//             +0x00  ulong index (next-free hint)
//             +0x04  CSWMiniGameObject *objects[255]
//
// Each slot is a CSWMiniGameObject* (or null). To classify by type we
// invoke the vtable slot:
//     vtable[0x14] = AsFollower
//     vtable[0x18] = AsPlayer
//     vtable[0x1c] = AsEnemy
//     vtable[0x20] = AsObstacle
// Each returns the same `this` pointer downcast to the requested
// concrete subclass, or null if it isn't that type. So enumerating
// "all obstacles" = walk 255 slots, call vtable[0x20] thiscall on each
// non-null slot, keep the non-null returns. We make two such passes
// per tick — one for obstacles (rocks/debris), one for enemies
// (accelerator pads).
//
// KOTOR 2: identical, and so is the 255. Three independent witnesses in its own
// code — CSWMiniGameObject's ctor @0x00833000 registers itself with
// `array[4 + mgo_index*4] = this`; the by-name lookup @0x00832eb0 walks
// `[array + i*4 + 4]` under `cmp i, 0xff / jge`; and the AsEnemy-filtered getter
// @0x00832e20 indexes the same way. The four downcast slots are unchanged too —
// the vtable-slot map pairs all six minigame classes with EXACT slot counts, and
// slots 5/6/7/8 (= +0x14/+0x18/+0x1c/+0x20) are self_return on precisely
// CSWTrackFollower / CSWMiniPlayer / CSWMiniEnemy / CSWMGObstacle respectively,
// return_zero everywhere else — the same fingerprint as KOTOR 1's.
const size_t kMgoArrayObjectsOffset         = acc::off::Same(0x4);
constexpr int    kMgoArraySlotCount             = 255;
constexpr size_t kVtableSlotAsObstacle          = 0x20;
constexpr size_t kVtableSlotAsEnemy             = 0x1c;

// Accelpads use the SAME multi-source pass as obstacles — every in-range
// pad gets its own continuously-playing loop.
//
// SUPERSEDED — nearest-only (one loop at a time). It was adopted because
// the first multi-pad pass blurred: under the old UNIFORM 1:9 compression
// lateral pan was squashed to ±2 m, so 3-4 pads all sounded centred and
// smeared together. With anisotropic projection (true lateral) each pad
// now pans to its real lane, so concurrent pads are spatially distinct.
// Nearest-only also caused the audible cue to teleport side-to-side at
// every hand-off and gave no approach ramp (a pad popped in already at
// full volume at ~112 m). Per-slot loops fix both: a passed pad dies
// behind you, a new one fades in faint at the far edge, and the pads
// between play continuously — confirmed via patch-20260620-114139.log
// analysis. If the stage gets cluttered, cap to the nearest N (the band
// already floors distant pads, so a cap loses little).
// Position retrieval for a CSWMiniEnemy (and any CSWTrackFollower):
// the engine's CSWTrackFollower::GetPosition @0x0066d5d0 walks
//   followers->models.data[0] → vtable[+0x64] → returns Vector* world pos
// We replicate that path here. Offset 0x68 is the CExoArrayList<undefined4>
// `models` field in CSWTrackFollower (after the CSWMiniGameObject base
// 0x60 + mini_game ptr 0x4 + field2_0x64 0x4).
// CSWTrackFollower combat/physics scalars (shared layout with the turret —
// turret_game.cpp reads sphere_radius at the same offset). sphere_radius is the
// engine's real hit primitive: the accelpad hit test is a swept sphere-vs-sphere
// at player.sphere_radius + pad.sphere_radius (swoop-accelpad-hit-model.md), so
// we read BOTH live and sum them for the CROSS scoring rather than hard-coding a
// per-track literal. speed gates the steering magnet off during the start hold.
// Both Same() on KOTOR 2 — they sit below the script-CResRef array that grew
// there, so the +0x30 shift the CSWMiniPlayer fields take does not reach them.
// sphere_radius has a direct witness: K2's SetSphereRadius @0x008352b0, reached
// from CSWTrackFollower::Load's "Sphere_Radius" read, writes [this+0x84].
const size_t kFollowerSphereRadiusOffset    = acc::off::Same(0x84);  // float
const size_t kFollowerHpOffset              = acc::off::Same(0x8c);  // u32 (mine-pass log; witnesses in minigame_swoop_race.cpp)
const size_t kFollowerSpeedOffset           = acc::off::Same(0x98);  // float

// ----- Lateral-pan diagnostic (added 2026-06-20) -----
//
// To tune the accelpad pan we need the lateral geometry the engine
// actually pans on, none of which was in the log before (pan tuning was
// blind): the listener (camera) X, the bike's own world X, the player's
// tunnel-frame lane X, and the nearest-ahead pad's lateral error +
// resulting pan angle. The open question this answers: does the chase
// camera strafe sideways with the bike — so aligning the bike centres
// the cue — or stay near lane centre, so steering barely moves the pan?
// Compare listenerX against bikeWorldX across a run: if they track, the
// pan reference is sound and the problem is the sample / concurrency; if
// listenerX stays put while bikeWorldX swings, the pan can never centre.
//
// Engine confirmed (docs/llm-docs/swoop-accelpad-hit-model.md): hit band
// is player.Sphere_Radius + pad.Sphere_Radius (Taris 6.0, Tatooine /
// Manaan 5.0) in world units; world-X tracks tunnel-X 1:1 (lane ±20).
// CSWMiniGame.player at +0x24; CSWMiniPlayer.offset (tunnel lane coord,
// x = lateral) at +0x1c4.
// Both mirror the declarations in minigame_swoop_race.cpp / minigame_aim.h —
// see those for the KOTOR 2 witnesses (CSWMiniGame is unshifted; CSWMiniPlayer
// carries the follower's +0x30).
const size_t kMiniGamePlayerOffset          = acc::off::Same(0x24);
const size_t kMiniPlayerOffsetVectorOffset  = acc::off::Pick(0x1c4, 0x1f4);
// KOTOR 2 jump velocity (CSWMiniGame +0x118, float, u/s; the jump watch in
// minigame_swoop_race.cpp has the witnesses). With offset.z it gives the
// remaining flight time, which the bar-aware guide turns into a landing point.
const size_t kMiniGameJumpVelocityOffset    = acc::off::Kotor2Only(0x118);
constexpr float  kRadToDeg                       = 57.2957795f;

// ----- KOTOR 2 hazard geometry (bounding-box classification) -----
//
// K2's tracks are not rock fields. The offline atlas of the three tracks'
// obstacle models (docs/llm-docs/swoop-k2-controls-and-jump.md section 7) and
// the first K2 round (patch-20260914-163810) showed three kinds of "obstacle":
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
//     / 2460 / 6002 — where nothing collides: round 4 (patch-20260915-075412)
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
// (patch-20260915-150225; 0.665–0.668 in every speed bucket). Every
// time-to-contact below is in REAL seconds through this factor; before it
// existed the "1.0 s" lead was really 1.5 s.
constexpr float kSpeedToWorldPerSec = 0.665f;
// Jump physics (onjump.ncs: SWMG_SetJumpSpeed(30), gravity 45.8): the arc
// z(t) = 30t - 22.9t^2 lands at kJumpAirSec and is above a 6.3-unit bar for
// t in [kJumpOverBarFromSec, kJumpOverBarUntilSec] after the press. Round 12
// logged 21 flights of 1.297–1.328 s at every speed.
constexpr float kJumpGravity          = 45.8f;
constexpr float kJumpAirSec           = 1.31f;
constexpr float kJumpOverBarFromSec   = 0.26f;
constexpr float kJumpOverBarUntilSec  = 1.05f;
constexpr float kAirborneZ            = 0.25f;  // offset.z above this = in the air
// The cue means "press NOW": it fires kJumpCueLeadSec before the bar, so the
// press window (0.26–1.05 s before the bar) opens 0.55 s after the beep and
// closes 1.34 s after it. Round 12's presses came 0.75–1.27 s after the beep
// (five bars), one of them just past the old window's end (MGC03, landed
// short of MGC04) — hence 0.1 s more lead than the 1.5 s that had been tuned
// by ear. (1.35 speed-units = 2.0 s real was tried in round 3: the player
// pressed within 0.1 s of the beep and landed just short of the bar.)
// The jump itself is the player's: the mod never presses it (an auto-jump was
// built and removed the same day at the user's request — cues inform, they
// do not play). If the bike is airborne from an early press and touches down
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
// middle ("you must jump"). The side alone was asked for by the player after
// round 6 (patch-20260915-090027); the distance after round 14 (a fixed hard
// pan said "left" for a 4 u lean and a 12 u swing alike).
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
constexpr const char* kObstacleWarnLoopResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopObstacleWarn);
// SUPERSEDED — earlier samples tried:
// "v_dur_shldred"  — Duros voice; routed to voice bus, way too quiet
// "mgs_warnbust"   — one-shot warning used before the loop refactor

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
//      never lurching as the pad passes. That kills the overshoot the
//      forward-compression builds had (pan inflating as forward shrank).
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
//      steeper gradient near centre — which is the point. 6 m (was 12 m,
//      tuned down 2026-06-20): the pan-diagnostic run showed ~10 of 22
//      pad misses were undershoots, stopping 6-11 units short because at
//      12 m "sounds centred" spanned wider than the ±5 unit catch band
//      (5 units off was only ~24°). At 6 m the same 5-unit error reads
//      ~40° and 1 unit reads ~9.5° (vs 4.8° before) — roughly double the
//      near-centre sensitivity, so the band between "aligned" and "close"
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
//
// SUPERSEDED:
//   - 1:9 UNIFORM compression — squashed lateral pan to ±2 m, too weak to
//     hit pads by intent (patch-20260620-104051.log).
//   - TRUE position + 5/200 band — too quiet (patch-20260620-111927.log).
//   - Forward 1:10 + 3/30 band — nearly silent (max_vol too small).
//   - Forward 1:15 + 15/30 band — loud, but compression inflated the pan
//     angle → overshoot ("steer, hit wall, it passes").
//   - TRUE position + flat band + manual volume — honest, no overshoot,
//     but true pan made far targets a ~2° whisper: not localisable
//     (patch-20260620-122348.log). Hence the fixed-depth pan reference.
constexpr float       kSwoopPanForwardRefM = 6.0f;
constexpr float       kSwoopFlatBandMaxM   = 350.0f;
constexpr float       kSwoopFlatBandMinM   = 400.0f;
constexpr int         kSwoopVolNearByte    = 127;
constexpr int         kSwoopVolFarByte     = 50;
// Pan depth for the accelpad STEERING GUIDE specifically (the obstacle cues keep
// the 6 m shared depth above). Opened to 10 m so the pan angle spreads MAGNITUDE
// across the lane — "how far to swing" — for the big alternating gates: at 10 m a
// 12 u offset reads ~50°, a 35 u offset ~74°, so far gates stay distinguishable
// instead of both saturating to hard-pan. The shared depth was tuned DOWN to 6 m
// for near-centre precision; the magnet now owns near centre, so the guide is
// free to optimise for far-gate magnitude. (Larger = more linear magnitude, less
// near-centre angle — fine, the deadzone + magnet cover near centre.)
constexpr float       kSwoopGuidePanForwardRefM = 10.0f;
// The guide rests (goes silent) only when you're essentially ON the pad — within
// this lateral error, roughly the catch radius — NOT across the whole assist
// gap. First cut silenced it within the full 12 u gap, but most steering is small
// nudges (< 12 u), so the tone fell silent during exactly the common case and
// you were "listening for something and hearing nothing". A small rest zone keeps
// the tone present for all real steering and only quiets when truly locked on.
constexpr float       kSwoopGuideSilenceU   = 5.0f;
// Volume floor for the guide so distant gates stay clearly audible — the shared
// distance curve ramps down toward ~⅓ at range, which let the tone blend into the
// race mix. Floored here so the guide always cuts through (the cue slider still
// scales it).
constexpr int         kSwoopGuideMinVolByte = 118;  // 90 until 2026-09-15: "a bit too quiet" in the K2 mix

// ----- Co-pilot discrete-command steering (experimental, 2026-06-21) --------
//
// The continuous panned guide (above) made the PLAYER the controller of a
// high-inertia, delayed plant — and that loop is unstable: the 2026-06-20
// 222843 log shows the bike overshooting and oscillating around even the easy
// first pad (reached the lane ~0.75 s early, then sailed 6 units past it). No
// re-encoding of "where to go" fixes that, because the instability is in the
// human-in-the-loop, not the information.
//
// The co-pilot takes the control DECISION off the player. Code runs the
// controller every tick and issues discrete, pre-timed commands:
//   coast = smoothed_vel * kCoastTicks  — where the bike drifts to if you
//                                         RELEASE the steer key now (your
//                                         reaction delay + the bike's inertial
//                                         stop, bundled into one lead term).
//   proj  = padErr - coast              — the lane miss AFTER that coast.
//   |proj| <= kReleaseBand → ALIGNED (release now; you coast onto the pad).
//   proj > 0  → steer RIGHT;  proj < 0 → steer LEFT.
// The player does only coarse hold/release; code owns the hard part (WHEN to
// stop). The loop stays closed in code (real bike X re-read each tick), so an
// early/late reaction just re-issues the command — robust to coast-model error.
enum {
    kSteerCmdLeft    = -1,
    kSteerCmdNone    = 0,
    kSteerCmdRight   = 1,
    kSteerCmdAligned = 2,
};
// Track-axis probe verdict — see TrackAxisUsable.
enum {
    kAxisUndecided = 0,
    kAxisAccepted  = 1,
    kAxisRejected  = 2,
};
// Lead term, in ticks of current lateral velocity — predicts where the bike
// drifts if you release the steer key now.
//
// Engine grounding (decompiled 2026-06-22, CSWMiniPlayer::{AdjustPosition,
// Control,DoInertia,KeepInTunnel}): swoop steering is a BANK LEVEL clamped to
// [-10,+10] (AdjustPosition.field10_0x1d0), NOT a momentum accumulator, and the
// only lateral "inertia" is collision-plane bump inertia (DoInertia, gated on
// CSWMiniGame::use_inertia and only non-zero at a wall). Mid-lane there is
// essentially no momentum to coast on. Live traces confirm it: lateral velocity
// reverses in ~5-8 ticks and the real post-release coast is ~1 unit — whereas
// the old vel*8 model predicted ~13 (patch-20260622 slot 26: predicted 12.8 u,
// actual ~1 u). That ~10x overestimate fired the "aligned/release" cue 10-20 u
// short of the pad on 29% of gates — the dominant miss cause (Flaw 1).
//
// So this is a small velocity lead, NOT a stopping-distance estimate. RAISE a
// little if the bike still overshoots; do NOT restore the old large value.
//
// Probe 2026-06-22: raised 1.0 -> 2.5. With 1.0 the controller held "steer" all
// the way to ~2u and the bike overshot to the wall on 32% of ticks (up from
// 22%). Releasing a bit earlier should cut the wall-slamming and still land
// inside the 8u catch (real coast ~1u, so an aligned call near ~6u still lands
// ~5u). If this reintroduces undershoot, step back toward ~1.5.
constexpr float       kCoastTicks    = 2.5f;
// ----- Predictive overshoot / wall cue (2026-06-22) -------------------------
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
// side-lane pad at 11 stays silent. Round 6 (patch-20260915-090027) drifted
// from 18 to 20 at speed 155 with no cue at all.
constexpr float       kWallLeadTicksK2  = 35.0f;   // ~1.2 s at 30 ticks/s (20 = 0.3-0.7 s before every post hit of round 11, not enough)
constexpr float       kWallEdgeOffsetK2U = 16.0f;

constexpr float       kWallMinVel       = 0.5f;   // ignore slow drift (u/tick)
constexpr ULONGLONG   kWallCueDebounceMs = 400;   // don't machine-gun while pinned
constexpr float       kWallCuePanM      = 5.0f;   // ±pan of the cue to the wall side
// KOTOR 2 wall LOOP. One thump at a predicted crossing was not enough: round
// 7 (patch-20260915-091337) took 22 of 26 hits on the posts — six arriving
// at the wall 0.7 s after a thump the player did not register, sixteen
// relaunching from a park at |x| = 20 (the first build's gear-0 "pinned"
// thump stopped on every relaunch, so it was one thump, silence, crash).
// Player's call (2026-09-15): warn earlier and continuously, no rail. So on
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
// and runs over them: round 9 (patch-20260915-142030) took four mines in one
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
// (patch-20260915-162510) drove into the MGC03 and MGC06 mines five times in
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
// Mine collision reach, MEASURED (round 14, patch-20260915-203307, `mine
// pass:` lines): every mine reports sphere_radius 2.0 like a pad, but a hit
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
// FALLBACK combined accel-pad hit radius (player.Sphere_Radius +
// pad.Sphere_Radius) for the CROSS hit= flag. The scoring now reads BOTH radii
// LIVE off the engine (player + nearest pad, at +0x84) and sums them, so the flag
// matches the real swept sphere-vs-sphere test on whatever track/mod is loaded —
// no per-track literal to keep in sync. This constant is used only if a radius
// read faults. Reference values: stock Taris 6 (player 3 + pad 3), stock
// Tatooine/Manaan 5, our widened Tatooine .are 8 (player 3 + pad 5).
// See swoop-accelpad-hit-model.md.
constexpr float       kAccelpadHitRadiusU = 8.0f;

// ============================================================================
// Module state. Single-threaded under the engine OnUpdate tick.
// ============================================================================
struct SpatialAudioState {
    // Per-slot looping cues. Sized to the global MGO array slot count
    // so we can key by slot without a parallel index map. An MGO slot
    // is either an obstacle or an enemy (the AsXxx vtable downcasts
    // return null for mismatches), never both, so the two arrays don't
    // overlap on any one slot. Auto-cleans at DLL unload via RAII.
    acc::audio::LoopSource obstacle_loops[kMgoArraySlotCount];
    // The panned steering-guide tone (acc_boost). Pan setpoint = the look-ahead
    // pure-pursuit controller in TickAccelpadCues; centred = on the line.
    acc::audio::LoopSource accelpad_line_loop;
    // Low-pass state for the panned steering offset (eased toward the damped,
    // deadzoned err each tick; snapped on pad handoff).
    // Lateral-velocity tracking for the PD damping term: bike world X last tick
    // and the smoothed per-tick lateral velocity.
    float prev_bike_x      = 0.0f;
    bool  have_prev_bike_x = false;
    float smoothed_vel     = 0.0f;
    // The pad slot targeted last tick — when it changes (a pad crossed) we snap
    // the pan instead of easing, so no stale carry-over into the next approach.
    int   prev_ahead_slot    = -1;
    // Co-pilot command state (experimental). steer_cmd is the last command
    // issued (kSteerCmd*); last_steer_tick_ms paced the (now retired) directional
    // tick. Retained for the steering-guide diagnostic.
    int       steer_cmd          = kSteerCmdNone;
    // Panned-guide aligned confirmation: whether the guide tone was sounding
    // (off-pad) last tick, so we fire one centred "on track, stop steering" blip
    // on the steering→locked transition. Re-arms when you steer off again.
    bool      guide_was_active    = false;
    // Next-gate preview: the ahead_slot we've already announced the follow-on
    // gate for, so the heads-up fires once per gate.
    // Predictive overshoot/wall cue debounce: last time we fired the early
    // wall-impact sound, so it doesn't machine-gun while the bike sits pinned.
    ULONGLONG last_wall_cue_ms    = 0;
    acc::audio::LoopSource wall_loop;     // K2 wall band, see kWallBandU
    int       wall_loop_side       = 0;   // -1 left, +1 right, 0 silent
    int       detour_slot          = -1;  // mine the guide is detouring, -1 none
    // Lateral steering magnetism: throttle for the ~1/s SwoopAssist heartbeat
    // log (the assist writes offset.x every in-band tick; we only log once a
    // second so normal play isn't spammed).
    ULONGLONG last_magnet_log_ms  = 0;
    // Crossing-event scoring: last tick's targeted gate + its lateral miss and
    // forward gap, so when the target changes we can interpolate the EXACT
    // lateral miss at fwdGap=0 (ground-truth hit, vs the noisy periodic copilot
    // samples). prev_ahead_slot above is reused as the gate id.
    bool      have_prev_ahead    = false;
    float     prev_ahead_posX    = 0.0f;
    float     prev_ahead_posY    = 0.0f;
    float     prev_ahead_padErr  = 0.0f;
    float     prev_ahead_fwdGap  = 0.0f;
    // Diagnostic guards for the first-pass inventory dumps. One log
    // entry per race describing every obstacle / accelpad seen.
    bool obstacle_diag_emitted = false;
    bool accelpad_diag_emitted = false;
    // KOTOR 2 hazard geometry, resolved once per obstacle slot per race (the
    // boxes are static; the part walk touches mesh vertices, not per-tick
    // material). POD — this file is SEH-heavy.
    HazardInfo hazards[kMgoArraySlotCount];
    // KOTOR 2 mines: enemies (CSWMiniEnemy, model mgf_mine*) that sit still in
    // the lane and drop the bike to gear 0 on contact. The pad sweep finds them
    // (same pool as the pads) and hands them to the obstacle sweep as blocks.
    int    mine_count = 0;
    int    mine_slot[kMgoArraySlotCount];
    Vector mine_pos[kMgoArraySlotCount];
    float  mine_radius[kMgoArraySlotCount];   // sphere_radius, live (0 = read faulted)
    // Mine-pass log (see kMinePassWindowU): per mine slot, the closest 3-D
    // approach of the current pass and the hp when the pass began.
    bool   mine_pass_open[kMgoArraySlotCount];
    float  mine_pass_min_d3[kMgoArraySlotCount];
    float  mine_pass_dx[kMgoArraySlotCount];
    float  mine_pass_dy[kMgoArraySlotCount];
    float  mine_pass_dz[kMgoArraySlotCount];
    int    mine_pass_hp0[kMgoArraySlotCount];
    float  ground_z = 40.0f;   // bike world z last seen on the ground (mine reach)
    // KOTOR 2 bars in world space (the obstacle sweep rebuilds this every
    // tick from the resolved boxes; the pad sweep, which runs first, reads the
    // previous tick's copy — the geometry is static). The bar-aware guide
    // (kBarFreeClearU) and the pair-aware jump lead (kPairMinWindowSec) read it.
    // has_gap / gap_x / gap_side: the free-side strip (kPostContactU block).
    struct BarInfo { int slot; float x0, x1, y0, y1; bool has_gap; float gap_x; int gap_side; };
    int     bar_count = 0;
    BarInfo bars[kMgoArraySlotCount];
    int     bar_detour_slot      = -1;   // bar the guide is skirting, -1 none
    int     bar_air_cleared_slot = -1;   // bar released mid-flight (landing past it)
    bool    bar_gap_diag_emitted = false;
    ParkedBar parked_bar         = ParkedBar::None;   // see GetParkedBarSituation
    // Airborne state last tick (offset.z) — the touchdown edge re-arms one
    // "jump again" beep for a bar still ahead.
    bool   was_airborne = false;
    // Track-axis probe (see TrackAxisUsable). Decided once per race from the
    // bike's own travel; until it is, the geometry cues stay silent.
    int    axis_state       = kAxisUndecided;
    bool   have_axis_origin = false;
    Vector axis_origin      = {0.0f, 0.0f, 0.0f};
};

extern SpatialAudioState g_state;

// Helpers shared across the files, by the file that defines them.

// minigame_swoop_audio.cpp

// ----- Next-gate preview (2026-06-21) ---------------------------------------
//
// The geometry proves the gates are reachable (worst 39 u lateral jump needs
// ~7 ticks; the forward gap gives 8-18) — what kills the tight ones is that the
// co-pilot only revealed the next gate AT the handoff, so the player's ~6-tick
// reaction delay ate the front of the window. Preview fixes that the way a
// sighted player does: a soft, single heads-up of the FOLLOWING gate's
// direction, fired once you're settled on the current gate, so the direction is
// pre-loaded before the loud steer-now ticks start. Same pitch language (low =
// left, high = right) but quieter — "get ready", not "act".

// Linear volume byte for a source at 3D distance `dist` within `range`:
// full (kSwoopVolNearByte) at the listener, ramping to kSwoopVolFarByte
// at the range edge. Clamped to the endpoints outside [0, range].
inline int SwoopVolumeByte(float dist, float range) {
    if (range <= 0.0f) return kSwoopVolNearByte;
    float t = dist / range;                 // 0 near .. 1 far
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    int vb = static_cast<int>(
        kSwoopVolNearByte + t * (kSwoopVolFarByte - kSwoopVolNearByte) + 0.5f);
    if (vb < kSwoopVolFarByte)  vb = kSwoopVolFarByte;
    if (vb > kSwoopVolNearByte) vb = kSwoopVolNearByte;
    return vb;
}
const char* ReadAurObjectName(void* aurObject);

// minigame_swoop_audio_obstacles.cpp
void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius);
void TickObstacleCues(void* miniGame);

// minigame_swoop_audio_accelpad.cpp
void TickAccelpadCues(void* miniGame);

}  // namespace acc::swoop_race
