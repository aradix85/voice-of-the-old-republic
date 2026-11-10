// Local bridge: /do, the actions an assistant can take in the game.

#include <cstdlib>
#include <string>

#include "bridge_internal.h"
#include "engine_area.h"
#include "engine_game.h"
#include "guidance_autowalk.h"
#include "interact_dispatch.h"
#include "key_inject.h"
#include "narrated_target.h"
#include "speech.h"

namespace acc::bridge {

namespace {

constexpr const char* kOk = "{\"ok\":true}";

std::string Error(const char* what) { return "{\"error\":" + Json(what) + "}"; }

// False when there is nothing to say.
bool SayNow(const std::string& utf8) {
    const std::wstring w = Utf8ToWide(utf8);
    if (w.empty()) return false;
    acc::speech::Speak(w.c_str(), /*interrupt=*/true);
    return true;
}

// The actions on one object, by its server handle.
std::string RunObjectAction(const std::string& action, const Params& params) {
    const uint32_t handle =
        static_cast<uint32_t>(std::strtoul(Param(params, "handle").c_str(), nullptr, 0));
    if (handle == 0) return Error("handle missing");
    void* obj = acc::engine::ResolveServerObjectHandle(handle);
    if (!obj) return Error("no object with that handle");

    if (action == "select") {
        acc::narrated_target::Stamp(obj, handle);
        const std::string name = SpokenName(obj);
        if (Param(params, "quiet") != "1") SayNow(name);
        return "{\"ok\":true,\"name\":" + Json(name) + "}";
    }
    if (action == "interact") {
        acc::narrated_target::Stamp(obj, handle);
        acc::interact::DispatchInteract(obj, handle, /*forceRadial=*/false);
        return kOk;
    }
    if (action == "use")
        return acc::guidance::UseObject(handle) ? kOk : Error("use refused");
    return Error("unknown action");
}

}  // namespace

std::string RunAction(const Params& params) {
    const std::string action = Param(params, "action");

    if (action == "say") {
        return SayNow(Param(params, "text")) ? kOk : Error("text missing");
    }
    if (action == "key") {
        const std::string sc = Param(params, "scancode");
        if (sc.empty()) return Error("scancode missing");
        acc::key_inject::Tap(static_cast<int>(std::strtol(sc.c_str(), nullptr, 0)),
                             Param(params, "extended") == "1");
        return kOk;
    }

    if (!acc::game::IsKotor1()) return Error("this action is KOTOR 1 only for now");

    if (action == "walk") {
        PlayerFrame f = ReadPlayerFrame();
        if (!f.ok) return Error("no player position");
        Vector dst = f.pos;
        dst.x = static_cast<float>(std::atof(Param(params, "x").c_str()));
        dst.y = static_cast<float>(std::atof(Param(params, "y").c_str()));
        return acc::guidance::WalkTo(dst) ? kOk : Error("walk refused");
    }
    return RunObjectAction(action, params);
}

}  // namespace acc::bridge
