// Internal to the examine_view module (examine_view*.cpp); no other file includes
// it. The public interface is examine_view.h. Here: what the files share.

#pragma once

#include "examine_view.h"

#include "strings.h"

#include <cstddef>
#include <cstdint>

namespace acc::examine_view {

// Pre-composed rows, rebuilt on each Up/Down step so hit points and distance
// stay live.
constexpr int kMaxRows = 64;   // ~10 fixed + 5 effects + 30 feats

struct State {
    bool      active        = false;
    int       focusIdx      = 0;
    int       rowCount      = 0;
    char      rows[kMaxRows][192];
    uint32_t  targetHandle  = 0;
    void*     targetObj     = nullptr;  // the last resolved server object
};

// examine_view.cpp
extern State g_state;

// Helpers shared across the files, by the file that defines them.

// examine_view_reads.cpp
bool IsSentinel(uint32_t handle);
void* ReadCreatureStats(void* serverCreature);
int ReadHpMax(void* serverCreature);
int ReadLevel(void* serverCreature);
int ReadDamageLevel(void* obj);
bool ReadBlindFlag(void* serverCreature);
acc::strings::Id DamageLevelStringId(int level);
int ReadFactionId(void* serverCreature);
acc::strings::Id FactionWordIdFor(int factionId);
int Read2DDistanceMeters(void* obj);
uint32_t ReadLastTargetHandle();

// examine_view_rows.cpp
int BuildRows();  // 0 when the target is gone

}  // namespace acc::examine_view
