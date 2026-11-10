// Internal to the examine_view module (examine_view*.cpp); no other file includes
// it. The public interface is examine_view.h. Here: what the files share.

#pragma once

#include "examine_view.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "strings.h"

namespace acc::examine_view {

// Flat array of pre-composed strings. Recomposed on each Up/Down step
// (cheap) to keep HP and distance live without per-tick churn.
constexpr int kMaxRows = 64;   // ~10 fixed + 5 effects + 30 feats

struct State {
    bool      active        = false;
    int       focusIdx      = 0;
    int       rowCount      = 0;
    char      rows[kMaxRows][192];
    uint32_t  targetHandle  = 0;
    void*     targetObj     = nullptr;  // last resolved server obj — for refresh
};

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
bool ReadEquippedItemNameAtSlot(void* serverCreature, size_t slotOffset, char* outBuf,
                                size_t outBufSize);
uint32_t ReadLastTargetHandle();

// examine_view_rows.cpp
int BuildRows();

}  // namespace acc::examine_view
