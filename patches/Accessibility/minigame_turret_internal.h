// Internal to the minigame_turret module (minigame_turret*.cpp); no other file includes
// it. The public interface is minigame_turret.h. Here: what the files share.

#pragma once

#include "minigame_turret.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "audio_loop.h"       // LoopSource — per-fighter 3D engine loop
#include "minigame_aim.h"     // shared offset read/write + magnetism math
#include "engine_offsets_select.h"

namespace acc::turret_game {

// SEH read primitives + minigame-object-array resolution are shared across
// the minigame TUs (see minigame_aim.h). Brought into unqualified scope so
// the dense reads below read as they did when each file had its own copy.
using acc::engine::ReadPtrOrNull;
using acc::engine::ReadU32Or0;
using acc::engine::ReadFloatOr0;
using acc::engine::TryReadVector;
using acc::minigame::ResolveMgoArray;
using acc::minigame::CallAsCast;
using acc::minigame::ReadFollowerPosition;

// ============================================================================
// Engine struct offsets (shared with minigame_swoop_race.cpp — see that file and
// docs/llm-docs/re/swkotor.exe.h for the full CSWMiniGame walk). The KOTOR 2
// witnesses for the CSWCArea / CSWMiniGame / CSWTrackFollower values are at
// the matching declarations in minigame_swoop_race.cpp (same struct, same
// fields); turret-only fields carry their own below.
// ============================================================================
const size_t kMiniGameVtableOffset     = acc::off::Same(0x00);
const size_t kMiniGamePlayerOffset     = acc::off::Same(0x24);   // CSWMiniPlayer*
const size_t kMiniGameEnemyCountOffset = acc::off::Same(0x30);
const size_t kMiniGameTypeOffset       = acc::off::Same(0x80);   // 1=swoop, 2=turret
                                                     // (CSWMiniGame::Load sets
                                                     // type ONLY to 1 or 2)

// ----- Approach-cue: MGO enemy-pool walk -----
//
// Same chain minigame_swoop_audio*.cpp walks:
//   AppManager(*kAddrAppManagerPtr) +0x4 -> CClientExoApp +0x4 ->
//   CClientExoAppInternal +0x0 -> CSWMiniGameObjectArray
//   (+0x00 index, +0x04 objects[255]). vtable[0x1c] = AsEnemy.
const size_t     kMgoArrayObjectsOffset        = acc::off::Same(0x4);
constexpr int    kMgoArraySlotCount            = 255;
constexpr size_t kVtableSlotAsEnemy            = 0x1c;
// CSWTrackFollower model list (for world position via the model wrapper's
// vtable[+0x64], mirroring CSWTrackFollower::GetPosition).
// Combat fields on CSWTrackFollower (== CSWMiniEnemy, which is just a single
// follower at offset 0; offsets confirmed against the explicit field9_0x88
// and field17_0x144 markers in swkotor.exe.h). Diagnostics only:
//   sphere_radius — hitbox radius, so the TRUE subtended half-angle the gun
//                   must fall inside is atan(radius/dist), not a fixed 6°.
//   hp/max_hp     — per-fighter health; an hp drop is a real engine-scored
//                   hit this tick, the ground truth for the actual hit cone.
//   invulnerability — i-frame timer; nonzero means hits won't register.
//
// KOTOR 2: all five sit BELOW the follower's script-CResRef array (K1 +0xa4,
// the array that grew from ten to thirteen entries), so none of them moved.
// sphere_radius / hp / max_hp / speed carry witnesses in
// minigame_swoop_race.cpp. invulnerability (+0x9c, the live timer that K1's
// GetIsInvulnerable @0x005CB440 tests) is bracketed by two witnessed
// neighbours: speed +0x98 below it, and the invince PERIOD at +0xa0 above it,
// which each game's setter writes at the same offset (K1 SetInvincibility
// @0x0066C920, K2's "Invince_Period" setter @0x00835650). Log-only read.
const size_t kFollowerSphereRadiusOffset = acc::off::Same(0x84);  // float
const size_t kFollowerHpOffset           = acc::off::Same(0x8c);  // int
const size_t kFollowerMaxHpOffset        = acc::off::Same(0x90);  // int
const size_t kFollowerSpeedOffset        = acc::off::Same(0x98);  // float (engine speed)
const size_t kFollowerInvulnOffset       = acc::off::Same(0x9c);  // float

// ----- Range model (calibrated on KOTOR 1, scaled per area) -----
// The distances the cues use were tuned on KOTOR 1: a 600 m census, a 130 m
// "killable" range cue, a 1.6 s bolt-flight limit (480 m at the K1 bolt
// speed of 300), and a 1/30 hum compression. They are DERIVED from what
// the engine says about the current area — bolt speed and lifespan off
// the player's gun bank, each target's radius and speed — through formulas
// whose constants are exactly those K1 values, so KOTOR 1 lands where it was
// tuned and the K2 areas get the same model with their own numbers.
//
//   Make horizon   t_make = 1.6 s * sqrt((r / 20) / (v / 74))
//       The lead extrapolates; how far ahead it can predict before the miss
//       exceeds the hitbox grows with the radius r and shrinks with the
//       target's speed v. Offline check (docs/llm-docs/k2-turret-model.md):
//       on K1's paths the median lead miss at 1.6 s is ~1.65 radii; Onderon's
//       wider, faster paths reach the same ratio at 1.2-1.6 s, which is what
//       the formula gives for their 114-194 u/s fighters and 30 m radius.
//       Clamped to [0.5 s, bolt lifespan].
//   Makeable       bolt flight time <= t_make            (full-autoaim gate)
//   Killable cue   min(6.5 * r, boltSpeed * t_make)      (K1: min(130, 480))
//       6.5 radii = the by-ear aiming limit (an 8.75 degree hitbox), the
//       second term = how far a bolt gets inside the prediction horizon.
//   Census         min(boltSpeed * lifespan, 1.25 * boltSpeed * 1.6 s)
//                                                        (K1: min(900, 600))
//   Hum            census edge -> 20 m source distance   (K1: 1/30)
//
// Resulting starting values: K1 130 / 480 / 600; Dxun (bolt 600, same paths
// as K1) 130 / 960 / 1200; Onderon (bolt 600, r 30) 195 / 720-960 / 1200.
constexpr float kK1BoltSpeed        = 300.0f;  // M12ab Bullet.Speed — fallback when the gun bank can't be read
constexpr float kK1BoltLife         = 3.0f;    // M12ab Bullet.Lifespan — same fallback
constexpr float kRefRadius          = 20.0f;   // K1 fighter Sphere_Radius
constexpr float kRefFighterSpeed    = 74.0f;   // K1 fighter path speed (median of M12ab's six tracks)
constexpr float kRefMakeHorizonSec  = 1.6f;    // K1-tuned bolt-flight limit at the reference radius/speed
constexpr float kMinMakeHorizonSec  = 0.5f;
constexpr float kKillRangePerRadius = 6.5f;    // K1's 130 m range cue / 20 m radius
constexpr float kCensusOverMake     = 1.25f;   // K1's 600 m census / 480 m makeable distance
constexpr float kHumFarSourceM      = 20.0f;   // census edge maps to this source distance
// A bolt whose whole reach (speed * lifespan) is below this is not a travelling
// bolt at all: 107PER's gun fires at 0.2 u/s for 4 s. Its hits are not scored
// by bolt flight, so the lead, the flight-time gate and the range limits are
// switched off for that area ("hitscan").
constexpr float kHitscanReach       = 10.0f;

// ----- Hum (approach cue on the LOCKED fighter) -----
// A continuous loop on the selected fighter at its REAL position (distance-
// compressed onto the engine's audible band), so its loudness tracks how close
// THAT fighter actually is — "it's approaching" — and it pans toward where the
// fighter really is. Distinct from the peg, which marks the lead/aim point and
// swells with aim error; the gap between the two IS the lead. Restored from the
// beatable build (which rode selected + nearest; now just the locked one).
// The compression is kHumFarSourceM / census range (see the range model): K1's
// 1/30 maps the ~560 m spawn distance to ~19 m (faint end of the 5-20 m band)
// and reaches the 5 m floor by ~150 m, so the hum grows louder all the way in.
// A hitscan area has no census range; its targets are a hangar's width away.
constexpr float kFighterMinSourceDistanceM  = 5.0f;
constexpr float kHitscanHumCompression      = 0.4f;   // 107PER: troopers 10-50 u -> 5-20 m (starting value)
constexpr const char* kFighterLoopResref    = "acc_turret_loop";

// ----- Targeting "peg" cue (the single unified cue; drives the selected fighter) -----
// The aim (aziZ azimuth, elevX elevation, both degrees) forms a gun-ray
// direction vector; the angle between it and the direction to a fighter is
// the true 3D aim error (verified: at the zero-error frame the ray passed
// ~1.5 m from the fighter, and on-target frames averaged a 5 m lateral miss).
//
// The cue plays continuously while a fighter is SELECTED (via Q/E), mapping
// the aim error to that fighter onto loudness (via source distance): faint
// when the aim is far off it, loudest when on target. So the player locks a
// target with Q/E, then swings until the tone peaks and fires.
//
// "On target" is DISTANCE-SCALED, not a fixed angle. The engine scores hits
// against the fighter's 20 m sphere_radius, which subtends atan(radius/dist):
// ~20° at 60 m but only ~3° at 400 m (live-measured: on-target rate 11% close
// vs 0% far). The whole hitbox cone reads clearly LOUD (a shot can connect),
// but loudness still rises gently to a PEAK at dead-centre — because with bolt
// travel a moving fighter can drift out of the sphere mid-flight if you fire
// from the edge, whereas a centre shot leaves a full-radius margin. So:
//   centre (0°) ............ kPegMinDist  (loudest)
//   hitbox edge (subtend) .. kPegEdgeDist (still loud — "you can hit here")
//   subtend + kPegRampDeg .. kPegMaxDist  (audible FLOOR — the beacon level)
// A fixed peak-at-0° was too strict up close; a flat plateau lost the centre
// gradient that the bolt-travel margin needs. The whole hitbox MUST stay
// clearly loud ("loud == a shot connects"): a version that rewarded only the
// hitbox CENTRE (not the full subtend) tested distinctly worse — it fought the
// player toward dead-centre when the engine would already score a hit, so the
// loud peak stopped meaning "you can hit". So onTarget stays == full subtend.
//
// The saturation problem in the sole-cue build was the GRADIENT, not the floor.
// Pan already does the coarse "which way" job over the full 360° swing, so
// loudness only needs to carry the FINE approach near centre. The 45° ramp we
// tried when this became the only cue spread the 9->5 climb so thinly that
// "half right" already sat near max — no usable warmer/colder near centre. So
// keep the ramp NARROW: the volume swing concentrates in the last stretch of
// the swing, where fine aiming happens. 25° is a touch steeper than the 30°
// the two-cue ("beatable") build used, because the peg now carries fine-centring
// alone (no hum to assist).
//   FULL RANGE: the engine curve has usable slope from ~1 m "right on top"
//   through ~20 m "just audible" (swoop's obstacle cue shows it). Mapping
//   aim error into only the gentle end (5->20 m) leaves dead-centre faint and
//   the whole swing compressed, so the ramp spans the full band: dead-centre
//   at 1 m (loudest), the quiet beacon floor at 20 m.
constexpr float kPegRampDeg   = 25.0f;  // fade width beyond the hitbox edge (narrow = steep near-centre swing)
constexpr float kPegMinDist   = 1.0f;   // dead-centre -> right on top -> loudest
constexpr float kPegEdgeDist  = 9.0f;   // hitbox edge -> clearly audible (mid)
constexpr float kPegMaxDist   = 20.0f;  // way off -> just-audible beacon floor
// Fallback on-target half-angle when the hitbox radius can't be read (no enemy
// ptr this tick). Also the value the diagnostic "onTarget" flag falls back to.
constexpr float kFallbackOnTargetDeg = 6.0f;
// Behind-gate. The continuous cue is SILENCED only when the selected target
// is BEHIND the aim — past the rear boundary, i.e. in the rear hemisphere. The
// full forward 180° stays audible, so forward awareness is intact; we only drop
// the targets a crosshair could never point at without first turning around.
// The player swings (or Q/E-cycles) to bring a rear target back into the
// forward half. ~90° is the true front/back boundary, so this also removes the
// stereo front/back mirror (a rear-left target can't masquerade as a front-left
// one when the rear half is silent) without narrowing awareness.
//
// HYSTERESIS: a fighter orbiting right at the boundary would flap the peg on
// and off every few ticks (a 60 s session logged 17 off / 18 on transitions),
// which — with the behind tick firing the SAME sample — read as mush rather
// than a clean tone. So the gate has a dead band: cross INTO behind only past
// kBehindEnterDeg, and back to front only once below kBehindExitDeg. State is
// latched in State::selected_behind (reset on (re)lock / out-of-view).
constexpr float kBehindEnterDeg = 95.0f;  // front -> behind above this
constexpr float kBehindExitDeg  = 85.0f;  // behind -> front below this
constexpr const char* kLockCueResref = "acc_turret_lock";

// ----- Rising-tick-rate crosshair -----
// The precision signal is the PULSE RATE, not loudness. Loudness-via-distance
// has a steep knee in the engine curve that can't track the range-scaled hit
// cone (it read "loud the moment I'm roughly on" well outside the cone at
// range). So off-target the cue PULSES, and the interval shrinks as the aim
// closes on the cone — slow pings far off, near-buzz at the edge — then goes
// SOLID (continuous tone) the instant the aim is inside the hitbox (onTarget).
// Solid == "a shot connects here", at ANY range, because onTarget is already
// scaled to the subtend. Off-target ticks carry direction via pan.
constexpr ULONGLONG kTickFastMs  = 90;    // just outside the cone — near-buzz
constexpr ULONGLONG kTickSlowMs  = 700;   // far off / behind — slow ping
constexpr float     kTickRampDeg = 90.0f; // angular span over which rate ramps:
// the FULL forward hemisphere. The aim is routinely 60-160° off a far target;
// a narrow ramp pinned the rate at slowest for the whole swing ("barely rises").
// Spanning ~90° means every degree of swinging toward the target speeds the
// pulse — the warmer/colder tracking signal — collapsing to solid in the cone.
constexpr float     kTickDist    = 8.0f;  // fixed pan distance for forward ticks
// The pulse uses a SHORT (~60 ms) percussive sample, NOT the ~1 s sustained
// acc_turret_lock the solid tone loops: one-shots of the long sample overlapped
// into a constant drone that masked the rate entirely (only pan changed). The
// tick is the same timbre family (sliced from the lock tone) so accelerating
// pulses audibly merge into the solid lock tone on-target.
constexpr const char* kTickCueResref = "acc_turret_tick";

// ----- Range-window cue (joining / leaving killable range) -----
// Far fighters (subtend ~2.6° past 250 m) are unhittable by ear; close ones
// (~20° subtend under 100 m) hit ~25% (live QC). So the playable loop is
// "track it in as it closes, fire when it's actually in range." This one-shot
// punctuates that boundary: it fires when the locked fighter crosses INTO
// killable range, and again when it LEAVES (leaving matters most — it says
// "window closed, stop firing / reacquire"). Positioned at the fighter so it
// pans toward it. A short, loud, distinct sample so it never blurs into the
// continuous crosshair tone. K1's 130 m was derived from the close-band
// 25%-hit boundary, nudged out a little for reaction time; the distance now
// comes from KillableRangeM (range model above), which reproduces it on K1.
constexpr const char* kRangeCueResref = "c_drdastro_atk1";
// LEAVE fires the marker twice (a double-tap) so it's unambiguous against the
// single-tap ENTER — leaving ("window closed, reacquire") is the event the
// player most needs to read at a glance. This is the gap to the 2nd tap; long
// enough that the short sample doesn't blur into one, tick-rate independent.
constexpr ULONGLONG kLeaveTapGapMs    = 180;

// NOTE (engine-confirmed): the aim field (+0x1c4 offset) IS the aim —
// CSWMiniPlayer::Control integrates it and pushes it into the rotating gun/camera
// models each tick. The earlier "+0x1c4 is a no-op for bolts" conclusion was an
// artifact of measuring through the broken bolt-travel fire line (~85° off). So
// aim-assist now WRITES offset directly (ReadAimLine / WriteOffset above), and
// the bolts/camera follow. The downstream targeting here (lead, makeability,
// switch-on-hit) was always correct and unblocks now that the crosshair is real.
// ============================================================================
// Module state. Single-threaded under the engine OnUpdate tick.
// ============================================================================
// What the engine says about the current turret area: the player's bolt
// ballistics and the aim limits. Read at entry (ReadArena) and retried each
// tick until the gun bank resolves; until then the K1 values stand in.
struct Arena {
    bool  ballistics_read = false;
    float bolt_speed      = kK1BoltSpeed;
    float bolt_life       = kK1BoltLife;
    bool  hitscan         = false;   // bolt reach < kHitscanReach (107PER)
    // Aim limits from CSWMiniPlayer's tunnel, in offset units (degrees):
    // azimuth = offset.z, elevation = offset.x. A free axis wraps 0..360
    // (KeepInTunnel's TunnelInfinite); a bounded one is clamped by the engine
    // to origin+neg .. origin+pos.
    bool  limits_read = false;
    bool  az_free = true;
    float az_lo = 0.0f, az_hi = 360.0f;
    bool  el_free = false;
    float el_lo = -90.0f, el_hi = 90.0f;
};

// Per-target values the cues need, read during the census. Fighters fill
// them from their CSWTrackFollower; area targets from their server object.
struct TargetInfo {
    int   hp          = -1;
    int   max_hp      = -1;
    float radius      = 0.0f;
    float engine_speed = 0.0f;   // 0 = not known (area targets)
};

struct State {
    bool      active           = false;
    void*     latched_mini_game = nullptr;
    void*     latched_vtable    = nullptr;
    ULONGLONG entered_at_ms     = 0;
    int       ticks_since_lost  = 0;
    Arena     arena;
    // Area-target mode (107PER): the minigame declared no enemies, so the
    // census walks the area's hostile creatures and non-plot placeables.
    // Those have object handles, not MGO slots, so each gets a pseudo-slot
    // in the same 0..254 space the per-slot tables below use. area_noun is
    // the engine's own (localised) name — "Sith trooper", "Explosive
    // barrel" — spoken in place of "Fighter"; empty for minigame fighters.
    bool      area_targets = false;
    uint32_t  area_handle[kMgoArraySlotCount]      = {};
    bool      area_is_creature[kMgoArraySlotCount] = {};
    char      area_noun[kMgoArraySlotCount][48]    = {};
    int       area_slot_count = 0;
    // Single continuous targeting "peg" tone: tracks the Q/E-SELECTED
    // fighter; its loudness rises as the aim ray closes on that fighter.
    acc::audio::LoopSource peg_cue;
    // Continuous "approach" hum on the locked fighter (real compressed position;
    // loudness = how close it is). See kFighterLoopResref.
    acc::audio::LoopSource hum_cue;
    // MGO slot of the fighter the player has locked via Q/E (-1 = none).
    // Explicit selection — the cue no longer auto-jumps between fighters.
    int selected_slot = -1;
    // CSWMiniGame.enemy_count (+0x30) seen last tick. Logged on change so a
    // kill (count drops) is distinguishable in the log from a fighter merely
    // flying out of cue range (count unchanged, slot returns later). -1 =
    // not yet sampled.
    int last_enemy_count = -1;
    // Stable display number per MGO slot (0 = unassigned). Assigned the first
    // time a slot is seen alive in the pool and never reused, so "Fighter N"
    // names the SAME physical fighter for the whole round — unlike the census
    // index, which is a per-tick distance rank. Survives the fighter leaving
    // cue range and (for the kill announce) outlives its destruction.
    int slot_number[kMgoArraySlotCount] = {0};
    int next_number = 1;
    // Per-slot kill/HP tracking across ticks, independent of selection — so we
    // announce EVERY downed fighter (not just the locked one) and can tell a
    // kill from a despawn. Filled by the Pass-1 walk, which visits every
    // fighter anyway. slot_alive_last = resolved AsEnemy last tick;
    // slot_hp_last = last-seen hp (-1 unknown); slot_damaged = ever seen hp
    // below max (so a fighter that vanishes undamaged reads as a despawn).
    bool slot_alive_last[kMgoArraySlotCount] = {false};
    int  slot_hp_last[kMgoArraySlotCount]    = {0};
    bool slot_damaged[kMgoArraySlotCount]    = {false};
    // Aim-assist heartbeat-log throttle (the assist writes the aim every tick;
    // we only log ~1/s so normal play isn't spammed).
    ULONGLONG assist_last_log_ms = 0;
    // Session accuracy telemetry: player shots (CSWMiniPlayer::Fire calls) and
    // fighter hits, for a hits/shots line at exit. Reset on turret entry.
    int shots_fired = 0;
    int fighter_hits = 0;
    // Rear-arc pinned pan (behind steering): the last chosen swing side. Latched
    // so a target sitting exactly behind (lateral ~0) doesn't flip-flop the pan
    // left/right tick to tick.
    bool      last_behind_swing_left = false;
    // Rising-tick-rate crosshair: timestamp of the last off-target pulse. The
    // pulse interval is recomputed each tick from the aim error; a pulse fires
    // when this many ms have elapsed. Solid tone (on-target) bypasses it.
    ULONGLONG last_tick_ms = 0;
    // On-target edge tracker for the selected fighter: true while the combined
    // aim was inside the hitbox last tick. A false->true transition fires the
    // one-shot "fire now" cue. Reset on (re)lock and when the target leaves the
    // forward view, so re-acquiring re-arms the cue.
    bool selected_on_target = false;
    // Behind-gate hysteresis latch: true while the selected fighter is in the
    // rear hemisphere (peg silenced, behind-tick steering). Crosses to true
    // above kBehindEnterDeg and back to false below kBehindExitDeg, so a target
    // orbiting near the 90° boundary doesn't flap the peg on/off. Reset to
    // false (front) on (re)lock and out-of-view so re-acquiring re-evaluates.
    bool selected_behind = false;
    // HP of the selected fighter last tick (-1 = unknown / just (re)selected).
    // A drop between ticks is a real engine-scored hit; logged with the aim
    // error at that frame to measure the actual hit cone. Reset on every
    // selection change so a new lock doesn't read as a phantom hit.
    int last_selected_hp = -1;
    // Killable-range edge tracker for the locked fighter: -1 unknown, 0 out of
    // range, 1 in range. A change fires the enter/leave range cue. On a fresh
    // lock AnnounceSelectedTarget baselines this to the target's actual state
    // (and fires ENTER immediately if it's already in range).
    int selected_in_range = -1;
    // Delayed second tap for a LEAVE range cue (double-tap = unambiguous vs the
    // single ENTER tap). 0 = none pending; otherwise the GetTickCount64 ms at
    // which the second tap fires, played at leave_tap_pos.
    ULONGLONG leave_tap_due_ms = 0;
    Vector    leave_tap_pos    = {0.0f, 0.0f, 0.0f};
    // Previous-tick LISTENER-RELATIVE position + timestamp of the selected
    // fighter, for the velocity estimate (v = Δrelpos/Δt). Working relative to
    // the listener folds ship motion into the velocity automatically, so the
    // intercept is directly "where to point the turret". Invalid until
    // have_last_pos. vel_ema is the smoothed relative velocity used for lead.
    Vector    last_selected_pos = {0.0f, 0.0f, 0.0f};
    ULONGLONG last_selected_pos_ms = 0;
    bool      have_last_pos = false;
    Vector    vel_ema = {0.0f, 0.0f, 0.0f};
    bool      have_vel = false;
    // Curve-aware lead: smoothed centripetal acceleration (the perpendicular part
    // of d(vel_ema)/dt), with a settle counter. Reset on (re)lock with have_vel.
    Vector    accel_ema     = {0.0f, 0.0f, 0.0f};
    bool      have_accel    = false;
    int       accel_samples = 0;
    // Full-autoaim makeability retarget: when the locked fighter went unmakeable
    // (0 = makeable/unknown), and when the player last cycled by hand (so an
    // auto-switch honours a deliberate Q/E pick for kManualGraceMs).
    ULONGLONG unmakeable_since_ms  = 0;
    ULONGLONG last_manual_cycle_ms = 0;
    ULONGLONG last_retarget_ms     = 0;  // last auto-switch, for the commit cooldown
    ULONGLONG last_hit_ms[kMgoArraySlotCount] = {};  // per-slot last damaging hit (invuln tracking)
    // LeadCheck self-measuring diagnostic: a deferred ring of intercept
    // predictions (world frame) tagged with their bolt-arrival time. When a probe
    // matures we compare it to the fighter's ACTUAL position then, logging the
    // linear-vs-curve miss distance — so the lead model is tunable/validatable
    // from the log alone, no sighted check. dueMs == 0 marks an empty slot.
    struct LeadProbe { ULONGLONG dueMs; int slot; Vector lin; Vector curve; };
    static constexpr int kLeadProbeCount = 16;
    LeadProbe lead_probes[kLeadProbeCount] = {};
    int       lead_probe_next = 0;
    ULONGLONG lead_probe_push_ms = 0;
    // ---- Aim-assist sign calibration (offset.az/.el -> world aim az/el). ----
    // The engine's offset->world rotation sign is fixed but a priori unknown, so
    // we learn d(worldAimAz)/d(offsetAz) and d(worldAimEl)/d(offsetEl) PASSIVELY
    // from any offset motion (player swing or our own assist writes) — target-
    // independent, converges in a couple of ticks. sign* in {-1,+1}, 0=unknown
    // (assume +1 and keep learning). vote* accumulates agreement for stability.
    float     calib_last_off_az = 0.0f, calib_last_off_el = 0.0f;
    float     calib_last_aim_az = 0.0f, calib_last_aim_el = 0.0f;
    bool      calib_have    = false;
    int       calib_sign_az = 0, calib_sign_el = 0;
    int       calib_vote_az = 0, calib_vote_el = 0;
    // ---- Per-session aim-quality counters (QC summary on exit). A clean,
    // bolt-travel-immune measure of how well the cue lets the player aim:
    // fraction of tracked frames with the aim ray inside the hitbox, by range
    // band. Reset on enter.
    int   qc_frames = 0;                  // TurretAim frames with a live aim
    int   qc_on_total = 0;                // within-hitbox frames (any range)
    int   qc_n[3]  = {0, 0, 0};           // frames per band: close/mid/far
    int   qc_on[3] = {0, 0, 0};           // within-hitbox per band
    float qc_min_err = 1e9f;              // best (smallest) errAngle seen
    float qc_sum_err = 0.0f;              // for mean errAngle
};

extern State g_state;

// Helpers shared across the files, by the file that defines them.

// minigame_turret_arena.cpp
void ReadArena(void* miniGame);
float MakeHorizonSec(float radius, float speed);
float CensusRangeM();
float KillableRangeM(float radius, float speed);
float HumCompression();
float TargetSpeed(const TargetInfo* info, float measured);
bool ReadAimLine(Vector& outDir, Vector& outOrigin);
bool ReadOffset(float& azOut, float& elOut);
void WriteOffset(float az, float el);
float AngleBetweenDeg(const Vector& a, const Vector& b);

// minigame_turret_census.cpp
int CensusIndexOf(const int occSlot[], int occCount, int slot);
int NumberForSlot(int slot);
void TickFighterCues();

// minigame_turret_targets.cpp
void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs);
void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now);
Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener);
void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true);
void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener);

// minigame_turret_peg.cpp
void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool);
float Wrap180(float d);

// minigame_turret_assist.cpp
void UpdateMotionEstimate(const Vector& P, ULONGLONG now);
void CalibrateOffsetSigns(float offAz, float offEl, float aimAz, float aimEl);
bool SteerGun(bool steer, float offAz, float offEl, float angle, float errAz, float errEl, bool fullAuto, float& gain);
void AutoaimRetarget(bool justLandedHit, bool makeable, float leadT, float crossingSpeed, float sd, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener);

}  // namespace acc::turret_game
