// Local bridge: /do, the actions an assistant can take in the game.

#include "bridge_internal.h"

namespace acc::bridge {

namespace {

uint32_t ParseHandle(const std::string& v) {
    if (v.empty()) return 0;
    return static_cast<uint32_t>(std::strtoul(v.c_str(), nullptr, 0));
}

}  // namespace

std::string Param(const Job& job, const char* key) {
    auto it = job.params.find(key);
    return it == job.params.end() ? std::string() : it->second;
}

std::string RunAction(const Job& job) {
    std::string action = Param(job, "action");

    if (action == "say") {
        std::wstring w = Utf8ToWide(Param(job, "text"));
        if (w.empty()) return "{\"error\":\"text missing\"}";
        acc::speech::Speak(w.c_str(), /*interrupt=*/true);
        return "{\"ok\":true}";
    }
    if (action == "key") {
        std::string sc = Param(job, "scancode");
        if (sc.empty()) return "{\"error\":\"scancode missing\"}";
        int code = static_cast<int>(std::strtol(sc.c_str(), nullptr, 0));
        bool ext = Param(job, "extended") == "1";
        acc::key_inject::Tap(code, ext);
        return "{\"ok\":true}";
    }

    if (!acc::game::IsKotor1())
        return "{\"error\":\"this action is KOTOR 1 only for now\"}";

    if (action == "walk") {
        PlayerFrame f = ReadPlayerFrame();
        if (!f.ok) return "{\"error\":\"no player position\"}";
        Vector dst = f.pos;
        dst.x = static_cast<float>(std::atof(Param(job, "x").c_str()));
        dst.y = static_cast<float>(std::atof(Param(job, "y").c_str()));
        bool ok = acc::guidance::WalkTo(dst);
        return ok ? "{\"ok\":true}" : "{\"error\":\"walk refused\"}";
    }

    uint32_t handle = ParseHandle(Param(job, "handle"));
    if (handle == 0) return "{\"error\":\"handle missing\"}";
    void* obj = acc::engine::ResolveServerObjectHandle(handle);
    if (!obj) return "{\"error\":\"no object with that handle\"}";

    if (action == "select") {
        acc::narrated_target::Stamp(obj, handle);
        std::string name = SpokenName(obj);
        if (Param(job, "quiet") != "1") {
            std::wstring w = Utf8ToWide(name);
            if (!w.empty()) acc::speech::Speak(w.c_str(), /*interrupt=*/true);
        }
        return "{\"ok\":true,\"name\":" + Json(name) + "}";
    }
    if (action == "interact") {
        acc::narrated_target::Stamp(obj, handle);
        acc::interact::DispatchInteract(obj, handle, /*forceRadial=*/false);
        return "{\"ok\":true}";
    }
    if (action == "use") {
        bool ok = acc::guidance::UseObject(handle);
        return ok ? "{\"ok\":true}" : "{\"error\":\"use refused\"}";
    }
    return "{\"error\":\"unknown action\"}";
}

}  // namespace acc::bridge
