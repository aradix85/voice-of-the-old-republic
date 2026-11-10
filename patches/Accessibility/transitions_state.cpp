// The state the transitions files share, defined in one place; each item is
// described where it is declared, in transitions_internal.h.

#include "transitions_internal.h"

#include "room_topology.h"      // single source of truth for perceptual-
                                // region labels (nav-graph decomposition)

namespace acc::transitions {

void* g_prev_area        = nullptr;
int   g_prev_cluster_id  = acc::room_topology::kClusterIdNone;
bool  g_module_load_pending = false;
char  g_prev_friendly_room_name[64] = {0};
int   g_pending_cluster_id    = acc::room_topology::kClusterIdNone;
int   g_pending_cluster_count = 0;
bool   g_pending_cluster_centre = false;
Vector g_pending_centre_pos     = {0.0f, 0.0f, 0.0f};
float  g_pending_centre_radius  = 0.0f;
float  g_pending_centre_best    = 0.0f;
DWORD  g_pending_centre_best_ms = 0;
std::string g_last_spoken_room_text;
Vector g_last_spoken_pos       = {0.0f, 0.0f, 0.0f};
bool   g_last_spoken_pos_valid = false;
std::string g_pending_platz_text;
DWORD  g_pending_platz_tick      = 0;
bool   g_pending_platz_valid     = false;
int    g_pending_platz_room      = -1;
Vector g_pending_platz_pos       = {0.0f, 0.0f, 0.0f};
float g_lm_enter_range_m = kLandmarkEnterRangeFloorM;
float g_lm_exit_range_m  = kLandmarkEnterRangeFloorM * kLandmarkExitRatio;
int   g_lm_prox_pending_idx     = -1;
int   g_lm_prox_pending_count   = 0;
int   g_lm_prox_last_spoken_idx = -1;
bool  g_gated_cluster_pending   = false;
DWORD g_gate_clear_since_ms     = 0;

}  // namespace acc::transitions
