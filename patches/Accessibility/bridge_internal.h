// Internal to the bridge module (bridge*.cpp); no other file includes
// it. The public interface is bridge.h. Here: what the files share.

#pragma once

#include "bridge.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>

#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cycle_state.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_game.h"
#include "engine_manager.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "exit_memory.h"
#include "filter_objects.h"
#include "guidance_autowalk.h"
#include "interact_dispatch.h"
#include "key_inject.h"
#include "menus_chain.h"
#include "menus_extract.h"
#include "log.h"
#include "mod_version.h"
#include "narrated_target.h"
#include "party_status.h"
#include "speech.h"
#include "same_name_suffix.h"
#include "strfmt.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

namespace acc::bridge {

struct Job {
    std::string                        path;
    std::map<std::string, std::string> params;
    std::string                        result;   // JSON body
    bool                               done = false;
};

// ---------------------------------------------------------------------------
// Main-thread readers (all engine access goes through the mod's SEH-guarded
// helpers, so nothing here needs __try).
struct PlayerFrame {
    bool   ok = false;
    Vector pos{0, 0, 0};
    float  yawDeg = 0;      // engine yaw: 0 = east, CCW
    bool   yawOk = false;
};

// Helpers shared across the files, by the file that defines them.

// bridge_http.cpp
std::string Json(const std::string& s);
std::string Num(double v, int decimals = 1);
void ParseQuery(const std::string& q, std::map<std::string, std::string>& out);

// bridge_describe.cpp
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
std::string BuildNearby(float radius);
std::string BuildOverview();
std::string BuildObjects();
std::string BuildRooms();

// bridge_actions.cpp
std::string Param(const Job& job, const char* key);
std::string RunAction(const Job& job);

}  // namespace acc::bridge
