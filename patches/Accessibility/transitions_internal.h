// Internal to the transitions module (transitions*.cpp); no other file includes
// it. The public interface is transitions.h. Here: what the files share.

#pragma once

#include "transitions.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace acc::transitions {

// The region last committed: a room_topology cluster id, kClusterIdNone
// until the first labelled cluster resolves. The trigger is the cluster, not
// the .lyt-room: room ids flip on most player steps in dense areas, while a
// cluster is a perceptual region (walking a corridor keeps the id, entering
// a junction flips it once). The gated refire speaks it again.
extern int g_prev_cluster_id;

// The cutscene-load latch (IsModuleLoadPending in transitions.h says why).
// Set by the SetMoveToModuleString hooks and NotifyExternalLoadStarting;
// cleared in Tick() the next time a fresh area pointer appears.
extern bool g_module_load_pending;

// Helpers shared across the files, by the file that defines them.

// transitions_centre.cpp
void TrackMotion(const Vector& pos);
void ResetMotion();
void ArmCentreRule(int clusterId, const acc::room_topology::ClusterInfo& info,
                   const Vector& pos, DWORD sinceMs);
bool CentreRuleReady(int clusterId, const Vector& pos);

// transitions_room_speech.cpp
void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos);
void NoteGatedClusterChange();
void TickPendingPlaza(void* area, const Vector& playerPos);
void TickGatedClusterRefire(void* area, const Vector& playerPos);
void ResetRoomSpeechState();

// transitions_landmarks.cpp
void RebuildLandmarkCache(void* area);
void TickLandmarkCacheRecheck(void* area, const Vector& playerPos);
void TickProximityLandmarks(const Vector& playerPos);
void ResetLandmarkProximityState();

}  // namespace acc::transitions
