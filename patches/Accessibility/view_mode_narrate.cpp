// What view mode says about the cursor: the region it enters and the nearest object under it.

#include "view_mode_internal.h"

#include "engine_area.h"
#include "filter_objects.h"
#include "interact_dispatch.h"
#include "log.h"
#include "narrated_target.h"
#include "strings.h"
#include "speech.h"
#include "transitions.h"
#include "room_topology.h"

namespace acc::view_mode {

namespace {

// Tier order matches the walking adapter: landmark → friendly room name
// → shape cache. False = no tier resolved; caller stays silent.
bool ResolveCursorRegionLabel(void* area, const Vector& cursor,
                              std::string& outBuf,
                              const char*& outSource) {
    outBuf.clear();
    outSource = "none";

    // 15m matches the walking adapter's enter/exit windows.
    constexpr float kLandmarkRangeM = 15.0f;
    {
        char   landmarkBuf[128] = {0};
        Vector landmarkPos;
        if (acc::transitions::FindLandmarkNear(
                cursor, kLandmarkRangeM,
                landmarkBuf, sizeof(landmarkBuf), landmarkPos) &&
            landmarkBuf[0] != '\0') {
            outBuf = landmarkBuf;
            outSource = "landmark";
            return true;
        }
    }

    int roomIdx = -1;
    acc::engine::GetRoomAtIndexed(area, cursor, roomIdx);
    if (roomIdx >= 0) {
        char nameBuf[128] = {0};
        if (acc::engine::GetRoomDisplayName(area, roomIdx,
                                            nameBuf, sizeof(nameBuf)) &&
            nameBuf[0] != '\0' &&
            !acc::transitions::IsResrefStyleRoomName(nameBuf)) {
            outBuf = nameBuf;
            outSource = "room_name";
            return true;
        }
    }

    std::string shapeBuf;
    int  shapeSig   = 0;
    int  clusterId  = acc::room_topology::kClusterIdNone;
    if (acc::room_topology::LookupAt(area, cursor,
                                     shapeBuf,
                                     shapeSig, clusterId,
                                     /*allowDiagLog=*/true,
                                     /*requireWallReachable=*/true,
                                     /*zTrusted=*/false) &&
        !shapeBuf.empty()) {
        outBuf = shapeBuf;
        outSource = "shape";
        return true;
    }
    return false;
}

}  // namespace

// Per-tick region announce. Text-equality dedup + 300ms hover-pause so
// micro-crossings on a room seam don't fire. World-speech-gated.
void AnnounceCursorRegion(void* area, const Vector& cursor) {
    if (!area) {
        g_state.region_pending_text.clear();
        g_state.region_pending_started_ms = 0;
        return;
    }
    if (acc::transitions::IsWorldSpeechGated()) {
        // Hold pending state — when the gate releases, a fresh same-region
        // classification must not spam.
        return;
    }

    std::string label;
    const char* source = "none";
    bool resolved = ResolveCursorRegionLabel(area, cursor, label, source);
    if (!resolved || label.empty()) {
        g_state.region_pending_text.clear();
        g_state.region_pending_started_ms = 0;
        return;
    }

    if (label == g_state.region_last_spoken_text) {
        // Already announced — silent until a differently-labelled region.
        g_state.region_pending_text.clear();
        g_state.region_pending_started_ms = 0;
        return;
    }

    DWORD now = GetTickCount();
    if (label == g_state.region_pending_text &&
        g_state.region_pending_started_ms != 0) {
        if (now - g_state.region_pending_started_ms >= kHoverPauseMs) {
            acc::transitions::SpeakRegionLabel(label.c_str());
            g_state.region_last_spoken_text = label;
            g_state.region_pending_text.clear();
            g_state.region_pending_started_ms = 0;
            acclog::Write("ViewMode",
                          "region speak src=%s text=\"%s\" cursor=(%.2f,%.2f,%.2f)",
                          source, label.c_str(),
                          cursor.x, cursor.y, cursor.z);
        }
        return;
    }

    // New pending region — arm the timer.
    g_state.region_pending_text = label;
    g_state.region_pending_started_ms = now;
}

void NarrateNearestObject(void* area, const Vector& cursor) {
    if (!area) {
        // Reset on area drop — stale handle would compare-equal across
        // transitions; stale obj would dangle into freed memory.
        g_state.hover_pending         = 0;
        g_state.hover_pending_obj     = nullptr;
        g_state.hover_pending_started = 0;
        return;
    }

    float radiusSq = kHoverRadiusMeters * kHoverRadiusMeters;
    float bestDistSq = radiusSq + 1.0f;
    void* bestObj = nullptr;
    uint32_t bestHandle = 0;
    acc::filter::CycleCategory bestCat =
        acc::filter::CycleCategory::Count_;

    acc::engine::AreaObjectIterator iter(area);
    void* obj = nullptr;
    while ((obj = iter.Next()) != nullptr) {
        acc::filter::CycleCategory cat =
            acc::filter::CycleCategory::Count_;
        bool matched = false;
        for (int c = 0; c < int(acc::filter::CycleCategory::Count_); ++c) {
            auto cc = static_cast<acc::filter::CycleCategory>(c);
            if (acc::filter::ObjectMatches(obj, cc)) {
                cat = cc;
                matched = true;
                break;
            }
        }
        if (!matched) continue;

        Vector pos;
        if (!acc::engine::GetObjectPosition(obj, pos)) continue;
        float distSq = DistXYZSq(pos, cursor);
        if (distSq > radiusSq) continue;
        if (distSq >= bestDistSq) continue;

        bestDistSq = distSq;
        bestObj    = obj;
        bestHandle = acc::engine::GetObjectHandle(obj);
        bestCat    = cat;
    }

    DWORD now = GetTickCount();

    // Reset the pending timer whenever the nearest changes — rapid sweeps
    // keep resetting so we stay silent until the cursor actually settles.
    if (bestHandle != g_state.hover_pending) {
        g_state.hover_pending         = bestHandle;
        g_state.hover_pending_obj     = bestObj;
        g_state.hover_pending_started = now;
    }

    if (bestHandle == 0) return;
    if (bestHandle == g_state.hover_last_spoken) return;
    if (now - g_state.hover_pending_started < kHoverPauseMs) return;

    char name[128] = "";
    if (!acc::engine::GetObjectName(bestObj, name, sizeof(name)) ||
        name[0] == '\0') {
        std::snprintf(name, sizeof(name), "%s",
                      acc::strings::Get(acc::filter::CategoryNameId(bestCat)));
    }

    acc::speech::Speak(name, /*interrupt=*/true);
    g_state.hover_last_spoken = bestHandle;

    // Unified activation slot — Enter / Shift+Enter / Shift+- / Ctrl+- /
    // `-` all act on whatever the user just heard.
    acc::narrated_target::Stamp(bestObj, bestHandle);

    Vector pos = { 0.0f, 0.0f, 0.0f };
    acc::engine::GetObjectPosition(bestObj, pos);
    acclog::Write("ViewMode", "hover narrate handle=0x%08x cat=%s name=[%s] "
        "dist=%.2f cursor=(%.2f,%.2f,%.2f) obj=(%.2f,%.2f,%.2f)",
        bestHandle, acc::filter::CategoryName(bestCat), name,
        std::sqrt(bestDistSq),
        cursor.x, cursor.y, cursor.z,
        pos.x, pos.y, pos.z);
}

}  // namespace acc::view_mode
