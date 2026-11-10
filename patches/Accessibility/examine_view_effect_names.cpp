// Spoken names for effects: the EFFECT_TYPES enum and the effecticon.2da
// rows, keyed by raw engine ids (not strings.h ids). KOTOR 1 names come from
// the tables here; KOTOR 2 names its icon rows itself through its TLK.

#include "examine_view.h"

#include <cstdint>

#include "engine_game.h"   // IsKotor2 — K2 icon names resolve via TLK
#include "engine_reads.h"  // LookupTlk

namespace acc::examine_view {

namespace {

// EFFECT_TYPES enum (110 entries) → display name. Unmapped types fall
// back to "Effect #N" in the caller.

const char* EffectNameEn(int type) {
    switch (type) {
        case 1:   return "Haste";
        case 2:   return "Damage resistance";
        case 3:   return "Slow";
        case 5:   return "Disease";
        case 7:   return "Regeneration";
        case 8:   return "State";
        case 10:  return "Attack bonus";
        case 11:  return "Attack penalty";
        case 12:  return "Damage reduction";
        case 13:  return "Damage bonus";
        case 14:  return "Damage penalty";
        case 15:  return "Temporary hit points";
        case 16:  return "Damage immunity";
        case 17:  return "Damage vulnerability";
        case 18:  return "Entangled";
        case 19:  return "Death";
        case 20:  return "Knocked down";
        case 21:  return "Deaf";
        case 22:  return "Immunity";
        case 25:  return "Arcane spell failure";
        case 26:  return "Saving-throw bonus";
        case 27:  return "Saving-throw penalty";
        case 28:  return "Movement speed bonus";
        case 29:  return "Movement speed penalty";
        case 33:  return "Force resistance bonus";
        case 34:  return "Force resistance penalty";
        case 35:  return "Poisoned";
        case 36:  return "Ability bonus";
        case 37:  return "Ability penalty";
        case 38:  return "Damage";
        case 39:  return "Heal";
        case 43:  return "Droid stunned";
        case 44:  return "Modify number of attacks";
        case 45:  return "Cursed";
        case 46:  return "Silenced";
        case 47:  return "Invisible";
        case 48:  return "Armor class bonus";
        case 49:  return "Armor class penalty";
        case 50:  return "Spell immunity";
        case 53:  return "Taunted";
        case 55:  return "Skill bonus";
        case 56:  return "Skill penalty";
        case 60:  return "Force push";
        case 61:  return "Damage shield";
        case 62:  return "Disguised";
        case 63:  return "Sanctuary";
        case 64:  return "Time stop";
        case 73:  return "Blinded";
        case 75:  return "Miss chance";
        case 76:  return "Concealment";
        case 82:  return "Negative level";
        case 84:  return "Wounding";
        case 87:  return "Disarmed";
        case 90:  return "Force drain";
        case 91:  return "Temporary force points";
        case 92:  return "Blaster deflection bonus";
        case 93:  return "Blaster deflection penalty";
        case 94:  return "Horrified";
        case 95:  return "Force point damage";
        case 96:  return "Force point heal";
        case 97:  return "Choked";
        case 99:  return "Psychic static";
        case 100: return "Lightsaber throw";
        case 101: return "Assured hit";
        case 104: return "Assured deflection";
        case 107: return "Force shield";
        case 108: return "Pure good powers";
        case 109: return "Pure evil powers";
        default:  return nullptr;
    }
}

// effecticon.2da row → display name. This is the SIGHTED icon row: the
// engine mirrors each applied EFFECTICON effect into the creature's
// effect-icon array (CSWSEffectListHandler::OnApplyEffectIcon reads the
// row's IconResRef/Priority/Good columns), and the portrait renders those
// icons. Speaking these names is exact sighted parity — one name per icon
// a sighted player sees, in the same priority order.
//
// Names are the game's own: each row label matches a spells.2da row's name.
// Hand deviations: abbreviations are expanded, droid item names drop the
// type tail (one icon serves all types), and row 59 (flash-mine stun, no
// spells row) is hand-labelled. Rows 0/60/61
// (NULL/alignment gauges) stay unmapped.

const char* EffectIconNameEn(int id) {
    switch (id) {
        case 1:  return "Affliction";
        case 2:  return "Burst of Speed";
        case 3:  return "Choke";
        case 4:  return "Disable Droid";
        case 5:  return "Destroy Droid";
        case 6:  return "Fear";
        case 7:  return "Force Armor";
        case 8:  return "Force Aura";
        case 9:  return "Force Immunity";
        case 10: return "Force Valor";
        case 11: return "Force Push";
        case 12: return "Force Shield";
        case 13: return "Force Wave";
        case 14: return "Force Whirlwind";
        case 15: return "Stasis";
        case 16: return "Horror";
        case 17: return "Insanity";
        case 18: return "Kill";
        case 19: return "Knight Valor";
        case 20: return "Knight Speed";
        case 21: return "Master Valor";
        case 22: return "Master Speed";
        case 23: return "Plague";
        case 24: return "Improved Energy Resistance";
        case 25: return "Force Resistance";
        case 26: return "Energy Resistance";
        case 27: return "Stasis Field";
        case 28: return "Slow";
        case 29: return "Stun";
        case 30: return "Stun Droid";
        case 31: return "Wound";
        case 32: return "Adrenaline Shot: Strength";
        case 33: return "Adrenaline Shot: Alacrity";
        case 34: return "Adrenaline Shot: Stamina";
        case 35: return "Hyper Adrenaline Shot: Strength";
        case 36: return "Hyper Adrenaline Shot: Alacrity";
        case 37: return "Hyper Adrenaline Shot: Stamina";
        case 38: return "Combat Shot: Battle Stimulant";
        case 39: return "Combat Shot: Hyper Battle Stimulant";
        case 40: return "Combat Shot: Speed Stimulant";
        case 41: return "Grenade: Stun";
        case 42: return "Grenade: Sonic";
        case 43: return "Grenade: Adhesive";
        case 44: return "Grenade: Cryoban";
        case 45: return "Energy Shield";
        case 46: return "Sith Energy Shield";
        case 47: return "Arkanian Energy Shield";
        case 48: return "Echani Shield";
        case 49: return "Mandalorian Melee Shield";
        case 50: return "Mandalorian Power Shield";
        case 51: return "Echani Dueling Shield";
        case 52: return "Yusanis' Dueling Shield";
        case 53: return "Proto-type Verpine Energy Shield";
        case 54: return "Droid Force Field";
        case 55: return "Droid Stun Ray";
        case 56: return "Droid Flame Thrower";
        case 57: return "Droid Carbonite Stream";
        case 58: return "Droid Gravity Generator";
        case 59: return "Flash mine";
        default: return nullptr;
    }
}

// ---------------------------------------------------------------------------
// KOTOR 2 effect-icon names.
//
// K2's effecticon.2da carries a `namestrref` column (K1's does not — its
// names were mined by hand from spells.2da, hence the K1 table above). So
// the K2 table is a row -> strref map, resolved through the dual-game TLK
// path at speak time.
// Rows 0/60/62 (NULL_ICON and the two alignment gauges) carry no strref
// and stay unmapped, matching the K1 tables' skip set.

constexpr uint32_t kEffectIconStrRefK2[] = {
    /*  0 */ 0,      266,    267,    268,    271,    272,    275,    276,
    /*  8 */ 277,    279,    281,    282,    283,    285,    286,    288,
    /* 16 */ 289,    290,    291,    292,    293,    295,    296,    297,
    /* 24 */ 299,    300,    301,    303,    304,    305,    306,    309,
    /* 32 */ 1263,   1271,   1272,   1273,   1274,   1275,   1264,   1276,
    /* 40 */ 1277,   31401,  31404,  31405,  31406,  32136,  32137,  32138,
    /* 48 */ 32139,  32140,  32141,  32142,  32143,  32144,  32184,  32183,
    /* 56 */ 32192,  32197,  32198,  32199,  0,      32201,  0,      108169,
    /* 64 */ 48363,  48364,  48365,  48399,  48400,  48401,  48101,  48102,
    /* 72 */ 48103,  48104,  48105,  48414,  48452,  48453,  48454,  48482,
    /* 80 */ 48483,  48484,  48494,  48495,  48496,  48500,  48501,  48502,
    /* 88 */ 48503,  48504,  48505,  48336,  106491, 32185,  32186,  32188,
    /* 96 */ 32190,  32191,  48668,  48669,  32200,  49137,  49138,  32202,
    /*104 */ 48356,  42426,  42429,  48402,  48403,  48404,  108170, 121910,
    /*112 */ 132463, 42429,
};

// Resolves into a file-static buffer: every caller copies the name into
// its own output before the next EffectIconName call (both consumers are
// single-threaded snprintf-join loops), so one slot is enough.
char g_tlkNameK2[128];

const char* EffectIconNameK2(int id) {
    constexpr int kRows =
        static_cast<int>(sizeof(kEffectIconStrRefK2) /
                         sizeof(kEffectIconStrRefK2[0]));
    if (id < 0 || id >= kRows) return nullptr;
    uint32_t strref = kEffectIconStrRefK2[id];
    if (strref == 0) return nullptr;
    if (!acc::engine::LookupTlk(strref, g_tlkNameK2, sizeof(g_tlkNameK2)) ||
        g_tlkNameK2[0] == '\0') {
        return nullptr;
    }
    return g_tlkNameK2;
}

}  // namespace

const char* EffectName(int type) {
    // EFFECT_TYPES numbering is IDENTICAL across both games for the shared
    // range: the K1 handler-table registration (InitializeEffects
    // @0x004e4a10) and the K2 twin (0x00593a30) assign matching handlers to
    // matching indices for 3..109 — EFFECTICON is 67 in both. K2 appends
    // 110..117; the only persistent named state among them is 111
    // (Fury / Wookiee Rage — its apply handler 0x00594f50 keys off the
    // Fury/Rage spell rows and writes the stats FuryDamageBonus field), so
    // it resolves via the K2 TLK like the icon names. 110/112..114 are
    // registered as null markers and 115..117 are transient mechanics
    // (instant damage / cleanup) that never sit in an effect list long
    // enough to be spoken; they fall through to the caller's "Effect #N".
    if (type == 111 && acc::game::IsKotor2()) {
        constexpr uint32_t kFuryNameStrRef = 48494;  // spells.2da FORCE_POWER_FURY
        if (acc::engine::LookupTlk(kFuryNameStrRef, g_tlkNameK2,
                                   sizeof(g_tlkNameK2)) &&
            g_tlkNameK2[0] != '\0') {
            return g_tlkNameK2;
        }
        return nullptr;
    }
    return EffectNameEn(type);
}

const char* EffectIconName(int iconId) {
    // KOTOR 2 keys a different (larger) effecticon.2da and names its rows
    // itself via namestrref; the K1 table names K1's rows and would name the
    // wrong effect on K2.
    if (acc::game::IsKotor2()) return EffectIconNameK2(iconId);
    return EffectIconNameEn(iconId);
}
}  // namespace acc::examine_view
