// The state the guidance_approach files share, defined in one place; each item is
// described where it is declared, in guidance_approach_internal.h.

#include "guidance_approach_internal.h"

namespace acc::guidance {

uint32_t g_refireHandle = 0;
DWORD    g_refireAt     = 0;
ApproachState g_st;

}  // namespace acc::guidance
