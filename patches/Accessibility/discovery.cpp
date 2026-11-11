#include "discovery.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "engine_area.h"      // tag / kind / position / iterator
#include "engine_offsets.h"   // Vector
#include "engine_player.h"    // GetPlayerServerCreature
#include "engine_scriptvar.h" // the save variables
#include "filter_objects.h"   // ObjectMatches / CycleCategory
#include "log.h"
#include "same_name_suffix.h" // narration::PositionLess
#include "strfmt.h"           // ToLowerAscii

namespace acc::discovery {

namespace {

// One save var per area, named "ACC_DISC_<areaTag>", holding the discovered
// keys for that area joined by ';'. The in-memory mirror is rebuilt on area
// change (deferred load) and re-serialized on each new discovery.
std::string              g_areaTag;     // module resref, lower case
std::string              g_varName;
std::vector<std::string> g_keys;
void*                    g_area    = nullptr;
bool                     g_loaded  = false;
acc::engine::PlayerVarsSettle g_settle;

// Hard cap so a pathological area can't grow the save var unbounded. ~400
// keys × ~35 chars stays well under the 16 KB ReadPlayerVar returns.
constexpr size_t kMaxKeys = 400;

// The area key is folded to lower case. GetCurrentAreaResName returns the
// module resref as the transition that loaded it spelled it, and the game's
// data is not consistent about that: Peragus reaches the hangar as "106PER"
// from the administration level and as "106per" from the fuel depot, so an
// unfolded name gives one module two discovery sets.
using acc::strfmt::ToLowerAscii;

// Eligible discovery categories (Item + generic creatures are out of scope).
const acc::filter::CycleCategory kEligible[] = {
    acc::filter::CycleCategory::Door,
    acc::filter::CycleCategory::Npc,
    acc::filter::CycleCategory::Container,
    acc::filter::CycleCategory::Landmark,
    acc::filter::CycleCategory::Transition,
};

acc::filter::CycleCategory Classify(void* obj) {
    for (auto c : kEligible) {
        if (acc::filter::ObjectMatches(obj, c)) return c;
    }
    return acc::filter::CycleCategory::Count_;
}

// Derive the locale-independent per-area key for `obj`, or return false when
// the object is ineligible (item / non-nav / non-unique NPC / no tag). See
// discovery.h for the key model.
bool DeriveKey(void* obj, void* area, std::string& outKey) {
    outKey.clear();
    if (!obj || !area) return false;

    acc::filter::CycleCategory cat = Classify(obj);
    if (cat == acc::filter::CycleCategory::Count_) return false;

    char tag[96] = "";
    if (!acc::engine::GetObjectTag(obj, tag, sizeof(tag)) || tag[0] == '\0') {
        return false;
    }

    if (cat == acc::filter::CycleCategory::Npc) {
        // Named NPC: eligible only when the tag is unique among area
        // creatures (generic/duplicate mobs share tags and are excluded).
        int sameTag = 0;
        acc::engine::AreaObjectIterator it(area);
        while (void* o = it.Next()) {
            if (!acc::filter::ObjectMatches(o, acc::filter::CycleCategory::Npc)) {
                continue;
            }
            char t[96];
            if (acc::engine::GetObjectTag(o, t, sizeof(t)) &&
                std::strcmp(t, tag) == 0) {
                ++sameTag;
            }
        }
        if (sameTag != 1) return false;  // not unique → generic creature
        outKey = std::string("N~") + tag;
        return true;
    }

    // Static object: tags repeat, so disambiguate with the north-to-south
    // ordinal among same-category, same-tag area peers.
    Vector p;
    if (!acc::engine::GetObjectPosition(obj, p)) return false;
    int rank = 1;  // 1-based, northmost = 1
    acc::engine::AreaObjectIterator it(area);
    while (void* o = it.Next()) {
        if (o == obj) continue;
        if (!acc::filter::ObjectMatches(o, cat)) continue;
        char t[96];
        if (!acc::engine::GetObjectTag(o, t, sizeof(t)) ||
            std::strcmp(t, tag) != 0) {
            continue;
        }
        Vector op;
        if (!acc::engine::GetObjectPosition(o, op)) continue;
        // The order of the spoken numbering, so the ordinal in the key is the
        // "north path 3" the player hears.
        if (acc::narration::PositionLess(op, p)) ++rank;
    }
    char suffix[16];
    std::snprintf(suffix, sizeof(suffix), "~%d", rank);
    outKey = std::string("S~") + tag + suffix;
    return true;
}

bool Contains(const std::string& key) {
    for (const auto& k : g_keys) {
        if (k == key) return true;
    }
    return false;
}

// Reads one save variable and appends its ';'-separated keys to g_keys,
// without duplicates and up to the cap. Returns how many it added.
int MergeVar(const std::string& varName) {
    const std::string value = acc::engine::ReadPlayerVar(varName);
    int added = 0;
    std::string cur;
    for (const char* p = value.c_str();; ++p) {
        if (*p == ';' || *p == '\0') {
            if (!cur.empty() && !Contains(cur) && g_keys.size() < kMaxKeys) {
                g_keys.push_back(cur);
                ++added;
            }
            cur.clear();
            if (*p == '\0') break;
        } else {
            cur.push_back(*p);
        }
    }
    return added;
}

void LoadFromSave() {
    g_keys.clear();
    if (g_varName.empty()) { g_loaded = true; return; }

    const int parsed = MergeVar(g_varName);
    g_loaded = true;
    acclog::Write("Discovery", "loaded set var=%s keys=%d", g_varName.c_str(), parsed);
}

void Persist() {
    std::string val;
    for (size_t i = 0; i < g_keys.size(); ++i) {
        if (i) val.push_back(';');
        val += g_keys[i];
    }
    acc::engine::WritePlayerVar(g_varName, val);
}

// ---- Auto-discovery seeds -------------------------------------------------
// Combat-critical placeables the player must be able to cycle to even if
// they never walked past them in a discovery phase — mid-fight there is no
// calm exploration window. Seeded into the discovered set right after the
// area's save var loads, keyed by object tag (tags extracted from the
// module rims with xoreos-tools, 2026-07-17).
//   sta_m45ab — deck-2 turret-defense computers ("Computerterminal")
//   sta_m45ac — battle-droid control terminals (Terminal Type A/D/E/F + MK)
//   sta_m45ad — Malak fight: the captive Jedi (tags captive2..captive8)
struct AreaSeed {
    const char*        areaKey;   // module resref, matches g_areaTag
    const char* const* tags;      // nullptr-terminated tag list
};

const char* const kSeedTagsStaM45ab[] = {
    "sta45_turretcomp", nullptr,
};
const char* const kSeedTagsStaM45ac[] = {
    "k45_plc_assdroid", "k45_plc_prbdroid", "k45_plc_excharge",
    "k45_plc_wardroid", "k45_plc_mk", nullptr,
};
const char* const kSeedTagsStaM45ad[] = {
    "sta_plc_captive2", "sta_plc_captive3", "sta_plc_captive4",
    "sta_plc_captive5", "sta_plc_captive6", "sta_plc_captive7",
    "sta_plc_captive8", nullptr,
};

const AreaSeed kAreaSeeds[] = {
    {"sta_m45ab", kSeedTagsStaM45ab},
    {"sta_m45ac", kSeedTagsStaM45ac},
    {"sta_m45ad", kSeedTagsStaM45ad},
};

// Record every area object whose tag is on the current area's seed list.
// Runs once per area reconciliation (right after LoadFromSave); Record's
// own dup check makes revisits no-ops.
void ApplySeeds() {
    const char* const* tags = nullptr;
    for (const auto& s : kAreaSeeds) {
        if (g_areaTag == s.areaKey) { tags = s.tags; break; }
    }
    if (!tags || !g_area) return;

    int seeded = 0;
    acc::engine::AreaObjectIterator it(g_area);
    while (void* obj = it.Next()) {
        char tag[96];
        if (!acc::engine::GetObjectTag(obj, tag, sizeof(tag)) || !tag[0]) {
            continue;
        }
        for (const char* const* t = tags; *t; ++t) {
            if (std::strcmp(tag, *t) == 0) {
                size_t before = g_keys.size();
                Record(obj);
                if (g_keys.size() > before) ++seeded;
                break;
            }
        }
    }
    if (seeded > 0) {
        acclog::Write("Discovery", "auto-seeded %d objects for area %s",
                      seeded, g_areaTag.c_str());
    }
}

}  // namespace

void OnAreaChanged(void* area) {
    if (!area) return;

    // Per-area key: the module resref (e.g. "manm26aa") — stable and
    // language-independent. The area GFF Tag defaults to "untitled" (useless as
    // a key, it collides every area), so it's only a last-resort fallback.
    char key[96] = "";
    bool haveKey = acc::engine::GetCurrentAreaResName(key, sizeof(key)) &&
                   key[0] != '\0';
    if (!haveKey) {
        haveKey = acc::engine::GetAreaTag(area, key, sizeof(key)) && key[0] != '\0';
    }

    const std::string raw   = haveKey ? key : "";
    const std::string canon = ToLowerAscii(raw);  // see ToLowerAscii above

    // Idempotent: same area pointer + same key → nothing to do.
    if (g_area == area && g_areaTag == canon) return;

    g_area       = area;
    g_areaTag    = canon;
    g_keys.clear();
    g_settle.Reset();

    if (!haveKey) {
        // No stable area key — treat as an empty, already-loaded set so
        // Record/IsDiscovered no-op cleanly rather than retrying forever.
        g_varName.clear();
        g_loaded = true;
        acclog::Write("Discovery", "area change -> no area key; discovery disabled here");
        return;
    }

    g_varName = std::string("ACC_DISC_") + canon;
    g_loaded  = false;  // deferred load in Tick()
    acclog::Write("Discovery",
                  "area change -> key=%s (engine spelling '%s') var=%s (load deferred)",
                  canon.c_str(), raw.c_str(), g_varName.c_str());
}

void Tick() {
    if (g_loaded || !g_area) return;
    if (!g_settle.Tick()) return;
    LoadFromSave();
    ApplySeeds();
}

void Record(void* gameObject) {
    if (!gameObject) return;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return;
    if (area != g_area) OnAreaChanged(area);  // self-sync if transitions hasn't fired
    // Not yet reconciled (the brief settle window after a load): drop rather
    // than record into an un-loaded set, which would clobber the saved data on
    // the next Persist. Discoveries in this window are re-narrated in normal
    // play, so the loss is immaterial.
    if (!g_loaded || g_varName.empty()) return;

    std::string key;
    if (!DeriveKey(gameObject, area, key) || key.empty()) return;
    if (Contains(key)) return;

    if (g_keys.size() >= kMaxKeys) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            acclog::Write("Discovery",
                          "key cap %zu reached for var=%s — further discoveries "
                          "not persisted", kMaxKeys, g_varName.c_str());
        }
        return;
    }

    g_keys.push_back(key);
    Persist();
    acclog::Write("Discovery", "record key=%s (total %zu) var=%s",
                  key.c_str(), g_keys.size(), g_varName.c_str());
}

bool IsDiscovered(void* gameObject) {
    if (!gameObject) return false;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;
    if (area != g_area) OnAreaChanged(area);
    if (!g_loaded) return false;  // reconciliation pending (Tick loads once settled)

    // Nothing discovered in this area yet — skip the (O(N) per object) key
    // derivation entirely. Common in the early game / first visit.
    if (g_keys.empty()) return false;

    std::string key;
    if (!DeriveKey(gameObject, area, key) || key.empty()) return false;
    return Contains(key);
}

}  // namespace acc::discovery
