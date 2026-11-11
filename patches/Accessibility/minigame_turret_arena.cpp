// The turret's world: the gun model and its aim line, the arena
// (tunnel bounds, bolt speed and life), the ranges derived from it (make,
// census, killable), target speed, the player's aim offset and angles.

#include "minigame_turret_internal.h"

#include "log.h"

namespace acc::turret_game {

namespace {

// ============================================================================
// THE CROSSHAIR + aim-assist — engine-grounded (decompiled; see
// docs/llm-docs/turret-minigame-model.md and build/re/turret-aim.decomp.txt).
//
// CSWMiniPlayer::Control integrates `offset` (+0x1c4) each tick
// (`offset += axis-velocity; KeepInTunnel`) and pushes offset.x/y/z straight
// into the RotatingModel gun/camera parts — so offset.x = elevation°,
// offset.z = azimuth°, and the gun/camera point exactly where offset says.
//
//   * CROSSHAIR (read): the gun model's `bullethook0` node world transform —
//     the literal bolt origin+direction FireGunCallback uses to spawn bolts.
//     Walk player -> follower.gun_banks[0] -> guns[0] (CAurObject) ->
//     gob.vtable+0x98(name, &pos, &orient). Forward = orient · base. This
//     REPLACES the old measured-bolt-travel fire_line (the ~85° artifact).
//   * AIM-ASSIST (write): set offset.x/.z toward the target. The engine
//     re-integrates offset next tick, so a per-tick write sticks. Replaces the
//     turret_steer WASD synthesis (which moved offset but was judged against
//     the broken bolt read, so was wrongly thought inert).
// ============================================================================
// Gun-walk offsets (CSWTrackFollower is the base of CSWMiniPlayer at +0).
//
// KOTOR 2 witnesses, each the same as K1:
//   gun_banks +0x74 / count +0x78 — CSWMiniPlayer::CreateGunBank's twin
//     @0x00837700 reads [this+0x78] and appends through `this+0x74` (K1
//     @0x0066F440 does the same at [EBP+0x74] / [EBP+0x78]).
//   guns +0x34 / count +0x38 — CSWMGGunBank::AddGun's twin @0x008404A0
//     (vtable slot 0, exact pair with K1 @0x006743B0) appends the new gun
//     through `this+0x34`, a CExoArrayList (data, size, ...), so size = +0x38.
//   bullet speed +0x44 / lifespan +0x4c — the bank embeds its CSWMGBullet at
//     +0x40 (both constructors, K1 @0x006746D0 / K2 @0x0083FBF0), and the
//     setters write the fields directly: K1 SetBulletSpeed @0x00673840 /
//     SetBulletLifeSpan @0x00673860, K2 @0x00840250 / @0x00840280.
//   GetPartPosition = Gob vtable slot 38 (+0x98) in both games (RTTI slot map;
//     K1 @0x0043F430 / K2 @0x00462B90 decompile to the same body:
//     FindPart(name) -> world position (3 floats) + orientation (4 floats)).
const size_t kFollowerGunBanksOffset   = acc::off::Same(0x74);  // CSWMGGunBank** array base
const size_t kFollowerGunBankCountOff  = acc::off::Same(0x78);  // int
const size_t kGunBankGunsDataOffset    = acc::off::Same(0x34);  // CExoArrayList data ptr (CAurObject*[])
const size_t kGunBankGunCountOffset    = acc::off::Same(0x38);  // int
const size_t kGunBankBulletSpeedOffset = acc::off::Same(0x44);  // float, units/s
const size_t kGunBankBulletLifeOffset  = acc::off::Same(0x4c);  // float, seconds
constexpr size_t kGobVtableGetPartXform    = 0x98;  // gob.vtable slot: GetPartTransform

// Aim limits: CSWMiniPlayer's tunnel, the bounds CSWMiniPlayer::KeepInTunnel
// applies to `offset` each tick. K1's KeepInTunnel @0x0066CB40 reads, per
// axis: tunnel_infinite (+0x1fc) nonzero -> wrap 0..360 on a turret; else
// clamp to origin (+0x208) + tunnel_neg (+0x1f0) .. origin + tunnel_pos
// (+0x1e4). K2's CSWMiniPlayer::Load twin @0x00836350 (vtable slot 11, exact
// pair) stores the three tunnel vectors at +0x214 / +0x220 / +0x22c, and its
// SetOrigin twin @0x00838300 writes +0x238 — each K1 + 0x30, the
// CSWTrackFollower growth every CSWMiniPlayer field carries.
const size_t kMiniPlayerTunnelPosOffset      = acc::off::Pick(0x1e4, 0x214);
const size_t kMiniPlayerTunnelNegOffset      = acc::off::Pick(0x1f0, 0x220);
const size_t kMiniPlayerTunnelInfiniteOffset = acc::off::Pick(0x1fc, 0x22c);
const size_t kMiniPlayerOriginOffset         = acc::off::Pick(0x208, 0x238);

// gob.vtable+0x98: int __thiscall(this, const char* partName, Vector* outPos,
// Quaternion* outOrient). Returns nonzero when the part exists.
struct Quat { float w, x, y, z; };
typedef int (__thiscall* PFN_GetPartTransform)(void*, const char*, Vector*, Quat*);

// Rotate a vector by a quaternion (w,x,y,z): v' = v + 2w(u×v) + 2(u×(u×v)),
// u = (x,y,z). Standard, no normalisation assumed beyond unit q.
Vector QuatRotate(const Quat& q, const Vector& v) {
    const Vector u = {q.x, q.y, q.z};
    const Vector uxv = {u.y * v.z - u.z * v.y,
                        u.z * v.x - u.x * v.z,
                        u.x * v.y - u.y * v.x};
    const Vector uxuxv = {u.y * uxv.z - u.z * uxv.y,
                          u.z * uxv.x - u.x * uxv.z,
                          u.x * uxv.y - u.y * uxv.x};
    return {v.x + 2.0f * (q.w * uxv.x + uxuxv.x),
            v.y + 2.0f * (q.w * uxv.y + uxuxv.y),
            v.z + 2.0f * (q.w * uxv.z + uxuxv.z)};
}

// Resolve the player's first gun bank (CSWMGGunBank*).
void* ResolveGunBank0(void* miniGame) {
    void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    if (!player) return nullptr;
    if (ReadU32Or0(player, kFollowerGunBankCountOff) == 0) return nullptr;
    void* banksBase = ReadPtrOrNull(player, kFollowerGunBanksOffset);  // CSWMGGunBank*[]
    if (!banksBase) return nullptr;
    return ReadPtrOrNull(banksBase, 0);
}

// Resolve the player's first gun model (CAurObject*) for the bullethook read.
void* ResolveGunModel() {
    void* bank0 = ResolveGunBank0(g_state.latched_mini_game);
    if (!bank0) return nullptr;
    if (ReadU32Or0(bank0, kGunBankGunCountOffset) == 0) return nullptr;
    void* gunsData = ReadPtrOrNull(bank0, kGunBankGunsDataOffset);    // CAurObject*[]
    if (!gunsData) return nullptr;
    return ReadPtrOrNull(gunsData, 0);  // guns[0] = CAurObject*
}

}  // namespace

// Read the area's ballistics and aim limits into g_state.arena. Each half is
// read once; a half that fails (gun bank or player not built yet during the
// entry transition) is retried on the next tick.
void ReadArena(void* miniGame) {
    Arena& a = g_state.arena;
    if (!a.ballistics_read) {
        void* bank0 = ResolveGunBank0(miniGame);
        const float speed = bank0 ? ReadFloatOr0(bank0, kGunBankBulletSpeedOffset) : 0.0f;
        const float life  = bank0 ? ReadFloatOr0(bank0, kGunBankBulletLifeOffset)  : 0.0f;
        // Lifespan > 0 is the "read a real bullet" test: the engine's setter
        // refuses non-positive lifespans, so a built bank always passes it.
        if (life > 0.0f && life < 1000.0f && speed >= 0.0f && speed < 100000.0f) {
            a.bolt_speed      = speed;
            a.bolt_life       = life;
            a.hitscan         = speed * life < kHitscanReach;
            a.ballistics_read = true;
            acclog::Write("Turret", "arena: bolt speed=%.2f life=%.2fs reach=%.0f%s",
                          speed, life, speed * life,
                          a.hitscan ? " -> HITSCAN (no lead, no range limits)" : "");
        }
    }
    if (!a.limits_read) {
        void* player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
        Vector pos, neg, inf, org;
        if (player &&
            TryReadVector(player, kMiniPlayerTunnelPosOffset, &pos) &&
            TryReadVector(player, kMiniPlayerTunnelNegOffset, &neg) &&
            TryReadVector(player, kMiniPlayerTunnelInfiniteOffset, &inf) &&
            TryReadVector(player, kMiniPlayerOriginOffset, &org)) {
            const bool  azFree = inf.z != 0.0f;
            const bool  elFree = inf.x != 0.0f;
            const float azLo = org.z + neg.z, azHi = org.z + pos.z;
            const float elLo = org.x + neg.x, elHi = org.x + pos.x;
            // A bounded axis with an empty or inverted range would pin the
            // aim — most likely a player not yet loaded. Leave the steering
            // unclamped and read again next tick.
            if ((!azFree && azHi <= azLo) || (!elFree && elHi <= elLo)) {
                acclog::Trace("Turret", "arena: aim limits not plausible yet (az %.1f..%.1f "
                              "el %.1f..%.1f) — retrying", azLo, azHi, elLo, elHi);
            } else {
                a.az_free = azFree; a.az_lo = azLo; a.az_hi = azHi;
                a.el_free = elFree; a.el_lo = elLo; a.el_hi = elHi;
                a.limits_read = true;
                acclog::Write("Turret", "arena: azimuth %s %.1f..%.1f, elevation %s %.1f..%.1f",
                              a.az_free ? "free" : "bounded", a.az_lo, a.az_hi,
                              a.el_free ? "free" : "bounded", a.el_lo, a.el_hi);
            }
        }
    }
}

// ---- Range model (see the constants block for the derivation). ----------

// How far ahead (seconds of bolt flight) the lead can predict a target of
// this radius and speed before its miss outgrows the hitbox.
float MakeHorizonSec(float radius, float speed) {
    if (radius <= 0.0f) radius = kRefRadius;
    if (speed  <= 1.0f) speed  = kRefFighterSpeed;
    float t = kRefMakeHorizonSec *
              std::sqrt((radius / kRefRadius) / (speed / kRefFighterSpeed));
    if (t < kMinMakeHorizonSec)       t = kMinMakeHorizonSec;
    if (t > g_state.arena.bolt_life)  t = g_state.arena.bolt_life;
    return t;
}

// Census / Q/E range. Unlimited in a hitscan area (the census there is the
// area's own target list).
float CensusRangeM() {
    const Arena& a = g_state.arena;
    if (a.hitscan) return 1.0e9f;
    const float reach = a.bolt_speed * a.bolt_life;
    const float make  = kCensusOverMake * a.bolt_speed * kRefMakeHorizonSec;
    return reach < make ? reach : make;
}

// Distance inside which the range cue says "you can hit this".
float KillableRangeM(float radius, float speed) {
    if (g_state.arena.hitscan) return CensusRangeM();
    if (radius <= 0.0f) radius = kRefRadius;
    const float byEar  = kKillRangePerRadius * radius;
    const float byBolt = g_state.arena.bolt_speed * MakeHorizonSec(radius, speed);
    return byEar < byBolt ? byEar : byBolt;
}

float HumCompression() {
    if (g_state.arena.hitscan) return kHitscanHumCompression;
    return kHumFarSourceM / CensusRangeM();
}

// The speed the range model should use for a target: the engine's track speed
// for a fighter, else the measured one.
float TargetSpeed(const TargetInfo* info, float measured) {
    return (info && info->engine_speed > 0.0f) ? info->engine_speed : measured;
}

// THE CROSSHAIR. Read the bullethook0 node's world position + orientation and
// return the fire-line direction (unit) and origin. False if the part/gun is
// unavailable this tick (cue holds). baseForward is the node's local "down the
// barrel" axis — KOTOR object/part forward is +Y, IN-GAME CONFIRMED:
// full-autoaim drove this crosshair onto fighters and cleared the wave (6 kills
// in ~13 s, 93% of frames on-target across all ranges), which only works if +Y
// is the true fire line. (The other local axes read as lateral/up: +X elevation
// ~0, +Z elevation ~85.)
bool ReadAimLine(Vector& outDir, Vector& outOrigin) {
    void* gun = ResolveGunModel();
    const auto getPartTransform = reinterpret_cast<PFN_GetPartTransform>(
        acc::engine::ReadVirtual(gun, kGobVtableGetPartXform));
    if (!getPartTransform) return false;
    __try {
        Vector pos = {0, 0, 0};
        Quat   q   = {1, 0, 0, 0};
        int found = getPartTransform(gun, "bullethook0", &pos, &q);
        if (found == 0) return false;
        const Vector baseForward = {0.0f, 1.0f, 0.0f};
        Vector dir = QuatRotate(q, baseForward);
        const float n = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (n < 1e-4f) return false;
        outDir = {dir.x / n, dir.y / n, dir.z / n};
        outOrigin = pos;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Read / write the aim field (CSWMiniPlayer.offset +0x1c4): x = elevation°,
// z = azimuth°. Writing it steers the gun (the engine re-integrates next tick).
bool ReadOffset(float& azOut, float& elOut) {
    void* player = ReadPtrOrNull(g_state.latched_mini_game, kMiniGamePlayerOffset);
    Vector v;
    if (!acc::minigame::ReadOffsetVector(player, v)) return false;
    elOut = v.x;   // offset.x = elevation
    azOut = v.z;   // offset.z = azimuth
    return true;
}

void WriteOffset(float az, float el) {
    void* player = ReadPtrOrNull(g_state.latched_mini_game, kMiniGamePlayerOffset);
    Vector v;
    if (!acc::minigame::ReadOffsetVector(player, v)) return;  // preserve offset.y
    v.x = el;   // offset.x = elevation
    v.z = az;   // offset.z = azimuth
    acc::minigame::WriteOffsetVector(player, v);
}

// Angle (degrees) between the aim ray and the (already normalised)
// direction to a target. 0 = dead on.
float AngleBetweenDeg(const Vector& a, const Vector& b) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z;
    if (dot > 1.0f) dot = 1.0f; else if (dot < -1.0f) dot = -1.0f;
    return std::acos(dot) * kRadToDeg;
}

}  // namespace acc::turret_game
