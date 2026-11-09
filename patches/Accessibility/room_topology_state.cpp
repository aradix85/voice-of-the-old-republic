// The state the room_topology files share, defined in one place; each item is
// described where it is declared, in room_topology_internal.h.

#include "room_topology_internal.h"

namespace acc::room_topology {

AreaGraph g_graph;
DoorStabilityState g_doors_stability;
int s_class_clear     = 0;
int s_class_door      = 0;
int s_class_blocked   = 0;
int s_caveat1_hits    = 0;
int s_caveat2_hits    = 0;
std::vector<ExitMark>* g_exit_sink = nullptr;
int s_uf_parent[kMaxNodes];

}  // namespace acc::room_topology
