// Internal to the accelerator-pad cues (minigame_swoop_audio_accelpad*.cpp); no
// other file includes it. Here: what those files share.

#pragma once

#include "minigame_swoop_audio_internal.h"

namespace acc::swoop_race {

// Below this forward speed the race has not launched: during the countdown the
// engine centres the bike and clobbers any offset written.
constexpr float kSwoopMinRaceSpeed = 1.0f;   // u/s

// One tick of the pad cues: the readings, the nearest pad ahead, and the line
// the guide tone and the magnet steer to.
struct PadTick {
    Vector    listener = {0.0f, 0.0f, 0.0f};
    void*     player = nullptr;
    Vector    bikePos = {0.0f, 0.0f, 0.0f};
    bool      bikeOk = false;
    Vector    tunnel = {0.0f, 0.0f, 0.0f};  // the bike's offset Vector (+0x1c4)
    bool      tunOk = false;
    float     refY = 0.0f;
    float     bikeSpeed = 0.0f;
    float     playerRadius = 0.0f;
    ULONGLONG now = 0;
    int       slotsSeen = 0;
    int       padsFound = 0;
    int       padsAhead = 0;
    int       minesFound = 0;
    int       aheadSlot = -1;
    Vector    aheadPos = {0.0f, 0.0f, 0.0f};
    float     aheadRadius = 0.0f;  // all pads share one sphere_radius
    bool      virtualTarget = false;
    bool      haveTarget = false;
    // padErr and fwdGap stay the true nearest-pad values (the crossing score
    // relies on them); the detours move only guidePos and guideErr.
    float     padErr = 0.0f;
    float     fwdGap = -1.0f;
    Vector    guidePos = {0.0f, 0.0f, 0.0f};
    float     guideErr = 0.0f;
    int       barDetourSlot = -1;
    float     barY = 0.0f;
    int       mineDetourSlot = -1;
    bool      inGear = false;
};

// Helpers shared across the files, by the file that defines them.

// minigame_swoop_audio_accelpad_detour.cpp
void ApplyBarDetour(void* miniGame, PadTick& t);
void ApplyMineDetour(PadTick& t);

// minigame_swoop_audio_accelpad_assist.cpp
void ApplyMagnet(PadTick& t);
void PredictWallCue(const PadTick& t);
void UpdateWallLoop(const PadTick& t);

}  // namespace acc::swoop_race
