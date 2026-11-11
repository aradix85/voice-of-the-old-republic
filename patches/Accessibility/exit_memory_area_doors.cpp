// Doors into other areas: which door led where (ACC_EXIT_<area>) and the
// map notes seen in each area (ACC_SUM_<area>), kept in the save, so two
// areas with the same name can be told apart.

#include "exit_memory_internal.h"

#include "cycle_state.h"
#include "engine_area.h"
#include "engine_player.h"     // position, yaw, party, player creature
#include "engine_scriptvar.h"  // visited regions in the save
#include "log.h"
#include "same_name_suffix.h"
#include "save_points.h"

namespace acc::exit_memory {

namespace {

// A remembered door spot belongs to a door into another area this close.
constexpr float kExitMatchM      = 6.0f;
// A map note this close to a door into another area names that way out
// ("To Lower City"), not something inside the area.
constexpr float kExitNoteM       = 10.0f;
constexpr DWORD  kSummaryEveryMs  = 3000;
constexpr int    kSummaryMaxNames = 4;
constexpr size_t kMaxExitEntries  = 60;
// A remembered spot this close (whole metres) is the same door: replaced.
constexpr long   kSameSpotM       = 2;

using acc::save_points::Spot;

std::string ExitVarName(const std::string& key) { return "ACC_EXIT_" + key; }

std::string SumVarName(const std::string& key)  { return "ACC_SUM_" + key; }

// The door spots of `areaKey` with the module each leads to.
std::vector<Spot> ReadExits(const std::string& areaKey) {
    if (areaKey.empty()) return {};
    return acc::save_points::Parse(
        acc::engine::ReadPlayerVar(ExitVarName(areaKey)).c_str());
}

// Remember that the door at `pos` in `areaKey` leads to module `toKey`,
// replacing whatever was remembered for that spot.
void RecordExit(const std::string& areaKey, const Vector& pos,
                const std::string& toKey) {
    if (areaKey.empty() || toKey.empty()) return;
    std::vector<Spot> all = ReadExits(areaKey);
    acc::save_points::Put(all, acc::save_points::At(pos, toKey), kSameSpotM,
                          kMaxExitEntries);
    acc::engine::WritePlayerVar(ExitVarName(areaKey), acc::save_points::Format(all));
}

// The module remembered for the door spot nearest `doorPos`, empty if none.
std::string ExitTargetAt(const std::string& areaKey, const Vector& doorPos) {
    const std::vector<Spot> all = ReadExits(areaKey);
    const Spot* s = acc::save_points::Nearest(all, doorPos, kExitMatchM);
    return s ? s->value : std::string();
}

// The map notes of the current area that are not a way out, as the cycles
// name them, joined: "Largo's Apartment, Yun's Apartment".
std::string CurrentAreaSummary(void* area) {
    acc::cycle::CategoryListing listing;
    if (!acc::cycle::BuildCategoryListing(CycleCategory::Landmark, listing)) {
        return std::string();
    }
    std::vector<std::string> names;
    for (int i = 0; i < listing.count; ++i) {
        if (listing.isPin[i] || !listing.objs[i]) continue;
        if (TransitionDoorNear(area, listing.positions[i], kExitNoteM,
                               std::string(), nullptr)) {
            continue;
        }
        char name[128] = {0};
        if (!acc::narration::GetSpokenName(listing.objs[i],
                                           CycleCategory::Landmark,
                                           name, sizeof(name)) ||
            !name[0]) {
            continue;
        }
        if (std::find(names.begin(), names.end(), name) == names.end()) {
            names.push_back(name);
        }
    }
    std::sort(names.begin(), names.end());
    std::string out;
    int shown = 0;
    for (const std::string& n : names) {
        if (shown == kSummaryMaxNames) break;
        if (!out.empty()) out += ", ";
        out += n;
        ++shown;
    }
    return out;
}

}  // namespace

// For a door into another area: the map notes of the area behind it, as
// last seen there. Empty when the player never went through it.
std::string TransitionDoorMemory(const Vector& doorPos) {
    if (g_areaKey.empty()) return std::string();
    const std::string to = ExitTargetAt(g_areaKey, doorPos);
    if (to.empty() || to == g_areaKey) return std::string();
    return acc::engine::ReadPlayerVar(SumVarName(to));
}

// Per tick: the player's spot (for the door left by), the arrival check
// that records which door joins which two areas, and this area's notes.
void TickAreaMemory() {
    if (!g_area || acc::engine::GetCurrentArea() != g_area) return;
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return;
    if (!g_saveLoaded) return;  // settling: the spot may still be stale
    g_lastPos     = p;
    g_haveLastPos = true;

    if (g_arrival.pending) {
        g_arrival.pending = false;
        // Only a real walk through a door: the player stands at a door of
        // this area that leads back to the area just left (a save loaded
        // elsewhere does not).
        Vector doorPos{};
        if (TransitionDoorNear(g_area, p, kExitMatchM, g_arrival.fromName,
                               &doorPos)) {
            RecordExit(g_areaKey, doorPos, g_arrival.fromKey);
            RecordExit(g_arrival.fromKey, g_arrival.fromPos, g_areaKey);
            acclog::Write("ExitMemory", "door at (%.0f,%.0f) here leads to %s; "
                          "door at (%.0f,%.0f) there leads here (%s)",
                          doorPos.x, doorPos.y, g_arrival.fromKey.c_str(),
                          g_arrival.fromPos.x, g_arrival.fromPos.y,
                          g_areaKey.c_str());
        } else {
            acclog::Write("ExitMemory", "arrival at (%.0f,%.0f) not at a door to "
                          "'%s': no door recorded", p.x, p.y,
                          g_arrival.fromName.c_str());
        }
    }

    const DWORD now = GetTickCount();
    if (now - g_summaryAtMs < kSummaryEveryMs) return;
    g_summaryAtMs = now;
    if (g_areaKey.empty()) return;
    const std::string s = CurrentAreaSummary(g_area);
    if (s.empty() || s == g_summarySaved) return;
    g_summarySaved = s;
    acc::engine::WritePlayerVar(SumVarName(g_areaKey), s);
    acclog::Write("ExitMemory", "%s: notes %s", g_areaKey.c_str(), s.c_str());
}

}  // namespace acc::exit_memory
