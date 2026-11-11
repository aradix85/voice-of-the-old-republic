// Which targets are there: the census of fighters and area targets with
// their numbers and hit points, a target that is gone, and the per-tick
// fighter cues that drive the selected target.

#include "minigame_turret_internal.h"

#include "audio_bus.h"        // PlayCue3D — one-shot lock-confirmation ping
#include "examine_view.h"     // IsHostileCreature — area-target census
#include "engine_area.h"      // GetCurrentArea + GetClientArea chain,
                              //   AreaObjectIterator + object accessors
#include "engine_reads.h"     // ReadObjectCurrentHitPoints, IsPlotObject
#include "engine_player.h"    // AppManager chain (kAddrAppManagerPtr ...) +
                              //   GetCameraPosition / GetPlayerPosition
#include "log.h"
#include "speech.h"
#include "strings.h"          // Get(TurretGameStarted/Controls/Ended)

namespace acc::turret_game {

namespace {

// ----- Area-target mode (107PER) -----
// Targets are server objects, not minigame followers: hostile creatures and
// non-plot placeables (the explosive barrels). The plot flag also excludes
// the hangar's static console and its indestructible target dummy, because
// both games' loaders set plot on every Static placeable. They have no
// sphere_radius, so the hitbox subtend uses a nominal radius — a starting
// value to tune against the first test round.
constexpr float kAreaTargetRadius = 1.0f;

// The per-tick target list (Q/E cycle, auto-pick, cues), nearest first, plus
// which slots are alive this tick so kills and escapes can be announced.
struct Census {
    int        count = 0;
    int        slot[kMgoArraySlotCount];
    float      dist[kMgoArraySlotCount];
    Vector     pos[kMgoArraySlotCount];
    TargetInfo info[kMgoArraySlotCount];
    bool       alive[kMgoArraySlotCount] = {};   // in the pool / area, any range
    bool       dead[kMgoArraySlotCount]  = {};   // area target seen at hp <= 0
    bool       selected_alive = false;           // the locked target is still alive
    // In place: the entry arrays are only read up to `count`.
    void Reset() {
        count = 0;
        selected_alive = false;
        for (int i = 0; i < kMgoArraySlotCount; ++i) { alive[i] = false; dead[i] = false; }
    }
};

void AddToCensus(Census& c, int slot, const Vector& pos, float distSq,
                 const TargetInfo& info) {
    c.slot[c.count] = slot;
    c.dist[c.count] = std::sqrt(distSq);
    c.pos[c.count]  = pos;
    c.info[c.count] = info;
    ++c.count;
}

// Per-slot HP bookkeeping shared by both census sources: a drop is a hit on
// that target (logged even when it isn't the locked one), and "ever damaged"
// lets the down-detector tell a kill from a despawn.
void TrackSlotHp(int slot, int hp, int maxHp) {
    if (g_state.slot_hp_last[slot] >= 0 && hp < g_state.slot_hp_last[slot]) {
        acclog::Write("TurretHit", "ANY slot=%d number=%d hp=%d->%d%s",
                      slot, g_state.slot_number[slot], g_state.slot_hp_last[slot], hp,
                      (slot == g_state.selected_slot) ? " (locked)" : "");
    }
    if (maxHp > 0 && hp < maxHp) g_state.slot_damaged[slot] = true;
    g_state.slot_hp_last[slot] = hp;
}

// Minigame fighters: every MGO slot that resolves AsEnemy. Fighters beyond
// the census range stay alive (no kill announce) but out of the Q/E list.
void CensusFighters(Census& c, const Vector& listener) {
    void* mgoArray = ResolveMgoArray();
    if (!mgoArray) return;
    const float range   = CensusRangeM();
    const float rangeSq = range * range;
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        void* slot = ReadPtrOrNull(mgoArray,
                                 kMgoArrayObjectsOffset +
                                 static_cast<size_t>(i) * sizeof(void*));
        void* enemy = slot ? CallAsCast(slot, kVtableSlotAsEnemy) : nullptr;
        if (!enemy) continue;

        // Alive in the pool — assign its stable number (idempotent) and note
        // if it's the locked fighter, BEFORE the range cull below.
        c.alive[i] = true;
        NumberForSlot(i);

        TargetInfo info;
        info.hp           = static_cast<int>(ReadU32Or0(enemy, kFollowerHpOffset));
        info.max_hp       = static_cast<int>(ReadU32Or0(enemy, kFollowerMaxHpOffset));
        info.radius       = ReadFloatOr0(enemy, kFollowerSphereRadiusOffset);
        info.engine_speed = ReadFloatOr0(enemy, kFollowerSpeedOffset);
        TrackSlotHp(i, info.hp, info.max_hp);
        if (i == g_state.selected_slot) c.selected_alive = true;

        Vector pos;
        if (!ReadFollowerPosition(enemy, pos)) continue;
        const float distSq = DistXYZSq(pos, listener);
        if (distSq > rangeSq) continue;
        AddToCensus(c, i, pos, distSq, info);
    }
}

// The pseudo-slot of an area target, assigned on first sight with the engine's
// name as its noun and a number counted per noun ("Explosive barrel 3",
// "Sith trooper 7"). -1 when all 255 slots are taken.
int AreaSlotFor(void* obj, uint32_t handle, bool isCreature) {
    for (int i = 0; i < g_state.area_slot_count; ++i) {
        if (g_state.area_handle[i] == handle) return i;
    }
    if (g_state.area_slot_count >= kMgoArraySlotCount) return -1;
    const int slot = g_state.area_slot_count++;
    g_state.area_handle[slot]      = handle;
    g_state.area_is_creature[slot] = isCreature;
    char* noun = g_state.area_noun[slot];
    if (!acc::engine::GetObjectBaseName(obj, noun, sizeof(g_state.area_noun[0])) || !noun[0]) {
        std::snprintf(noun, sizeof(g_state.area_noun[0]), "%s",
                      acc::strings::Get(acc::strings::Id::TurretUnitTarget));
    }
    int sameNoun = 0;
    for (int i = 0; i < slot; ++i) {
        if (std::strcmp(g_state.area_noun[i], noun) == 0) ++sameNoun;
    }
    g_state.slot_number[slot] = sameNoun + 1;
    acclog::Write("Turret", "area target: slot=%d handle=0x%08x %s [%s] number=%d",
                  slot, handle, isCreature ? "creature" : "placeable", noun,
                  g_state.slot_number[slot]);
    return slot;
}

// Area-target mode (107PER): hostile creatures and non-plot placeables in the
// player's area. A creature at hp <= 0 is a corpse — dead, not a target.
void CensusAreaTargets(Census& c, const Vector& listener) {
    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    acc::engine::AreaObjectIterator iter(area);
    void* obj = nullptr;
    while ((obj = iter.Next()) != nullptr) {
        const int kind = acc::engine::GetObjectKind(obj);
        const bool isCreature =
            kind == static_cast<int>(acc::engine::GameObjectKind::Creature);
        const bool isPlaceable =
            kind == static_cast<int>(acc::engine::GameObjectKind::Placeable);
        if (!isCreature && !isPlaceable) continue;
        if (acc::engine::IsPlotObject(obj)) continue;
        if (isCreature && !acc::examine_view::IsHostileCreature(obj)) continue;

        const int slot = AreaSlotFor(obj, acc::engine::GetObjectHandle(obj), isCreature);
        if (slot < 0) continue;

        TargetInfo info;
        info.hp     = acc::engine::ReadObjectCurrentHitPoints(obj);
        info.radius = kAreaTargetRadius;
        if (info.hp <= 0) {
            c.dead[slot] = true;
            continue;
        }
        c.alive[slot] = true;
        TrackSlotHp(slot, info.hp, -1);
        if (slot == g_state.selected_slot) c.selected_alive = true;

        Vector pos;
        if (!acc::engine::GetObjectPosition(obj, pos)) continue;
        AddToCensus(c, slot, pos, DistXYZSq(pos, listener), info);
    }
}

// Speak a target that left the field: destroyed, or — an area creature that
// vanished while alive (107PER's troopers are removed when they reach the
// ship) — got away.
void AnnounceTargetGone(int slot, bool diedOnScreen) {
    const int num = g_state.slot_number[slot];
    if (num <= 0) return;
    char buf[128];
    const char* noun = g_state.area_noun[slot];
    const bool escaped = noun[0] && g_state.area_is_creature[slot] && !diedOnScreen;
    if (!noun[0]) {
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(acc::strings::Id::FmtTurretDestroyed), num);
    } else {
        std::snprintf(buf, sizeof(buf),
                      acc::strings::Get(escaped ? acc::strings::Id::FmtTurretEscapedNamed
                                                : acc::strings::Id::FmtTurretDestroyedNamed),
                      noun, num);
    }
    acc::speech::Speak(buf, /*interrupt=*/true);
    const char* why = escaped ? "(vanished alive -> escaped)"
                    : (g_state.slot_damaged[slot] || diedOnScreen) ? "(damaged -> kill)"
                    : "(no damage seen -> despawn?)";
    acclog::Write("Turret", "down: slot=%d number=%d %s", slot, num, why);
}

}  // namespace

// Find a slot's index in the (nearest-first) census, or -1.
int CensusIndexOf(const int occSlot[], int occCount, int slot) {
    for (int a = 0; a < occCount; ++a) if (occSlot[a] == slot) return a;
    return -1;
}

// Stable "Fighter N" number for an MGO slot, assigned on first request and
// fixed for the round (see State::slot_number).
int NumberForSlot(int slot) {
    if (slot < 0 || slot >= kMgoArraySlotCount) return 0;
    if (g_state.slot_number[slot] == 0) {
        g_state.slot_number[slot] = g_state.next_number++;
    }
    return g_state.slot_number[slot];
}

void TickFighterCues() {
    Vector listener;
    if (!acc::engine::GetCameraPosition(listener) &&
        !acc::engine::GetPlayerPosition(listener)) {
        return;
    }

    // THE CROSSHAIR: read the gun's bullethook0 node world direction (the
    // literal fire line). Available from frame 1 — no waiting for bolt travel.
    // Holds the cue (haveAim=false) only if the gun model isn't resolvable this
    // tick (transient during entry).
    Vector       aimDir  = {0, 0, 0};
    Vector       aimOrigin = listener;
    const bool   haveAim = ReadAimLine(aimDir, aimOrigin);

    // ---- Census (for the Q/E cycle and every cue). Static: four 255-entry
    // arrays are more than the engine tick's stack should carry.
    static Census c;
    c.Reset();
    if (g_state.area_targets) CensusAreaTargets(c, listener);
    else                      CensusFighters(c, listener);

    // Sort the census nearest-first so Q/E cycles near->far and the auto-pick
    // lands on the nearest (insertion sort; the count is small). The spoken
    // number is the slot's stable number, independent of this order.
    for (int a = 1; a < c.count; ++a) {
        const int        s = c.slot[a];
        const float      d = c.dist[a];
        const Vector     p = c.pos[a];
        const TargetInfo t = c.info[a];
        int b = a - 1;
        while (b >= 0 && c.dist[b] > d) {
            c.slot[b + 1] = c.slot[b];
            c.dist[b + 1] = c.dist[b];
            c.pos[b + 1]  = c.pos[b];
            c.info[b + 1] = c.info[b];
            --b;
        }
        c.slot[b + 1] = s;
        c.dist[b + 1] = d;
        c.pos[b + 1]  = p;
        c.info[b + 1] = t;
    }

    // Announce EVERY target that left the field this tick, by its stable
    // number — not just the locked one (the old logic only saw the selected
    // slot, so kills you weren't locked onto were silent). The player holds
    // the fire key during the turret game; with NVDA's "speech interrupt for
    // typed characters" on, held keys cancel speech. Each kill interrupts,
    // so two kills within ~1 s clobber — but hearing the last beats hearing
    // none.
    // slot_damaged tells a real kill from a possible despawn; fighters announce
    // both as destroyed and log the distinction.
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        if (g_state.slot_alive_last[i] && !c.alive[i]) {
            AnnounceTargetGone(i, c.dead[i]);
            g_state.slot_damaged[i] = false;
            g_state.slot_hp_last[i] = -1;
        }
        g_state.slot_alive_last[i] = c.alive[i];
    }

    // Fire the delayed second tap of a LEAVE cue (the double-tap that marks
    // "window closed"). Runs every tick, independent of selection, so it lands
    // even if the locked fighter has since left view.
    if (g_state.leave_tap_due_ms != 0 &&
        GetTickCount64() >= g_state.leave_tap_due_ms) {
        acc::audio::PlayCue3D(kRangeCueResref, g_state.leave_tap_pos);
        g_state.leave_tap_due_ms = 0;
    }

    // Q/E selects the locked target; the single peg cue tracks it (beacon when
    // off-aim, swelling on-target) and auto-advances to the nearest fighter
    // when the locked one is destroyed. This is the only continuous cue now —
    // the ambient per-fighter hum was dropped (mostly ignored in play-testing).
    HandleTargetCycle(c.slot, c.dist, c.pos, c.info, c.count, listener);
    // The aim geometry is measured from the camera, which sits a unit or two
    // off the gun's barrel. Irrelevant against fighters hundreds of units out,
    // but 107PER's troopers are 10-40 units away with a 1-unit radius, where
    // that offset is a whole hitbox — so a hitscan area aims from the barrel.
    const Vector& aimFrom = (g_state.arena.hitscan && haveAim) ? aimOrigin : listener;
    DriveSelectedPeg(c.slot, c.dist, c.pos, c.info, c.count, aimDir, haveAim,
                     aimFrom, c.selected_alive);

    // Hum: continuous loop on the LOCKED fighter at its real (compressed)
    // position — loudness = how close that fighter is, pan = where it really
    // is. Read AFTER DriveSelectedPeg so it tracks the final selection this
    // tick (incl. any auto-advance). Ungated by the behind-gate on purpose:
    // approach awareness matters even when the target is behind (the peg stays
    // front-only as the crosshair). Stops when the locked fighter isn't in the
    // census (out of range / none selected).
    {
        const int hidx = CensusIndexOf(c.slot, c.count, g_state.selected_slot);
        if (hidx >= 0) {
            const float realDist = c.dist[hidx];
            Vector humPos = c.pos[hidx];
            if (realDist > 0.0f) {
                float srcDist = realDist * HumCompression();
                if (srcDist < kFighterMinSourceDistanceM)
                    srcDist = kFighterMinSourceDistanceM;
                const float kk = srcDist / realDist;
                humPos.x = listener.x + (c.pos[hidx].x - listener.x) * kk;
                humPos.y = listener.y + (c.pos[hidx].y - listener.y) * kk;
                humPos.z = listener.z + (c.pos[hidx].z - listener.z) * kk;
            }
            if (g_state.hum_cue.IsActive()) {
                g_state.hum_cue.UpdatePosition(humPos);
            } else if (g_state.hum_cue.Start(kFighterLoopResref, humPos)) {
                acclog::Write("Turret", "hum on locked slot=%d realDist=%.0fm",
                              g_state.selected_slot, realDist);
            }
        } else if (g_state.hum_cue.IsActive()) {
            g_state.hum_cue.Stop();
            acclog::Write("Turret", "hum off (locked not in census)");
        }
    }

    acclog::Trace("Turret", "%s census: %d in range (range %.0fm)",
                  g_state.area_targets ? "area-target" : "fighter",
                  c.count, CensusRangeM());
}

}  // namespace acc::turret_game
