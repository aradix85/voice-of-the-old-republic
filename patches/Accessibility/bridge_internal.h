// Internal to the bridge module (bridge*.cpp); no other file includes
// it. The public interface is bridge.h. Here: what the main-thread readers
// share. All engine access goes through the mod's SEH-guarded helpers, so
// none of them needs __try.

#pragma once

#include <string>

#include "bridge_http.h"
#include "engine_offsets_types.h"
#include "filter_objects.h"

namespace acc::bridge {

constexpr const char* kNotInWorld =
    "{\"error\":\"no player position (not in the world?)\"}";

struct PlayerFrame {
    bool   ok = false;
    Vector pos{0, 0, 0};
    float  yawDeg = 0;      // engine yaw: 0 = east, CCW
    bool   yawOk = false;
};

// bridge_describe.cpp
std::string GameFields();   // "game" and "mod_version", no braces
std::string ToUtf8(const char* text);
std::wstring Utf8ToWide(const std::string& s);
std::string JsonGame(const char* gameText);
const char* Dir8(float compassDeg);
PlayerFrame ReadPlayerFrame();
std::string SpokenName(void* obj);
std::string RelativeJson(const PlayerFrame& f, const Vector& p);
const char* CategoryKey(acc::filter::CycleCategory c);

// bridge_state.cpp
std::string BuildSnapshot();
std::string BuildMenu();

// bridge_area.cpp
std::string BuildNearby(const Params& params);
std::string BuildOverview();
std::string BuildObjects();
std::string BuildRooms();

// bridge_actions.cpp
std::string RunAction(const Params& params);

}  // namespace acc::bridge
