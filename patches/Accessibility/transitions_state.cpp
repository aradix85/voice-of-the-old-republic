// The state the transitions files share, defined in one place; each item is
// described where it is declared, in transitions_internal.h.

#include "transitions_internal.h"

namespace acc::transitions {

int  g_prev_cluster_id     = acc::room_topology::kClusterIdNone;
bool g_module_load_pending = false;

}  // namespace acc::transitions
