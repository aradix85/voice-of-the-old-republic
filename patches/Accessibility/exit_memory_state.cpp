// The state the exit-memory modules share, defined in one place. Each item
// is described where it is declared, in exit_memory_internal.h.

#include "exit_memory_internal.h"

namespace acc::exit_memory {

void*         g_area = nullptr;
std::string   g_areaKey;
std::set<int> g_visited;
int           g_current  = kClusterIdNone;
int           g_previous = kClusterIdNone;
Vector      g_lastPos = {0.0f, 0.0f, 0.0f};
bool        g_haveLastPos = false;
Arrival     g_arrival;
DWORD       g_summaryAtMs = 0;
std::string g_summarySaved;
bool  g_saveLoaded     = false;
ExploreChain g_chain;

}  // namespace acc::exit_memory
