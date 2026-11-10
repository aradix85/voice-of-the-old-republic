// Local bridge: how game things read in the JSON answers: game text as
// UTF-8, names as the mod speaks them, distance and direction from the
// player.

#include <windows.h>

#include <cmath>
#include <string>

#include "bridge_internal.h"
#include "engine_area.h"
#include "engine_compass.h"
#include "engine_game.h"
#include "engine_player.h"
#include "mod_version.h"
#include "same_name_suffix.h"
#include "speech.h"

namespace acc::bridge {

namespace {

const char* kDir8[8] = {"north", "northeast", "east", "southeast",
                        "south", "southwest", "west", "northwest"};

// Compass bearing (0 = north, clockwise) of the vector dx,dy in the engine
// frame (+X east, +Y north).
float BearingDeg(float dx, float dy) {
    float engineYaw = std::atan2(dy, dx) * 57.29577951308232f;
    return acc::engine::EngineYawToCompass(engineYaw);
}

}  // namespace

std::string GameFields() {
    return "\"game\":" + Json(acc::game::TitleName()) +
           ",\"mod_version\":" + Json(std::string(acc::kModVersion) + "+bridge");
}

// Game codepage (what the engine and the speech path hand us) -> UTF-8.
std::string ToUtf8(const char* text) {
    if (!text || !*text) return std::string();
    UINT cp = acc::speech::GetSpeechCodepage();
    int wlen = MultiByteToWideChar(cp, 0, text, -1, nullptr, 0);
    if (wlen <= 0) return std::string(text);
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(cp, 0, text, -1, &w[0], wlen);
    int ulen = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0,
                                   nullptr, nullptr);
    if (ulen <= 0) return std::string(text);
    std::string u(static_cast<size_t>(ulen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &u[0], ulen, nullptr,
                        nullptr);
    if (!u.empty() && u.back() == '\0') u.pop_back();
    return u;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return std::wstring();
    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], wlen);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

std::string JsonGame(const char* gameText) { return Json(ToUtf8(gameText)); }

const char* Dir8(float compassDeg) {
    return kDir8[acc::engine::CompassToSector(compassDeg)];
}

PlayerFrame ReadPlayerFrame() {
    PlayerFrame f;
    f.ok = acc::engine::GetPlayerPosition(f.pos);
    f.yawOk = acc::engine::GetPlayerYawDegrees(f.yawDeg);
    return f;
}

// Name exactly as the in-world cycle speaks it (ordinal + state words),
// falling back to the plain engine name.
std::string SpokenName(void* obj) {
    char buf[256] = {0};
    for (int c = 0; c < static_cast<int>(acc::filter::CycleCategory::Count_);
         ++c) {
        auto cat = static_cast<acc::filter::CycleCategory>(c);
        if (acc::filter::ObjectMatches(obj, cat) &&
            acc::narration::GetSpokenName(obj, cat, buf, sizeof(buf)) &&
            buf[0]) {
            return ToUtf8(buf);
        }
    }
    buf[0] = 0;
    if (acc::engine::GetObjectName(obj, buf, sizeof(buf)) && buf[0])
        return ToUtf8(buf);
    return "?";
}

std::string RelativeJson(const PlayerFrame& f, const Vector& p) {
    if (!f.ok) return "null";
    float dx = p.x - f.pos.x, dy = p.y - f.pos.y;
    float bearing = BearingDeg(dx, dy);
    std::string s = "{\"distance\":" + Num(DistXY(f.pos, p)) +
                    ",\"bearing\":" + Num(bearing, 0) +
                    ",\"direction\":" + Json(Dir8(bearing));
    if (f.yawOk) {
        int clock = acc::engine::ClockPosition(f.yawDeg, dx, dy);
        s += ",\"clock\":" + std::to_string(clock);
    }
    s += "}";
    return s;
}

const char* CategoryKey(acc::filter::CycleCategory c) {
    using C = acc::filter::CycleCategory;
    switch (c) {
        case C::Door:       return "door";
        case C::Npc:        return "creature";
        case C::Container:  return "container";
        case C::Item:       return "item";
        case C::Landmark:   return "landmark";
        case C::Transition: return "transition";
        default:            return "other";
    }
}

}  // namespace acc::bridge
