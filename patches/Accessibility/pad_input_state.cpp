// The state the pad_input files share, defined in one place; each item is
// described where it is declared, in pad_input_internal.h.

#include "pad_input_internal.h"

namespace acc::pad {

bool g_ltHeld = false;
bool g_rtHeld = false;
bool g_ltChorded = false;
bool g_rtChorded = false;
bool g_pendingLtSolo = false;
bool g_pendingRtSolo = false;
bool g_pendingHelp   = false;
EnginePad g_enginePad = EnginePad::Unknown;
int       g_probeCode     = 0;
int       g_probeSlot     = -1;
ULONGLONG g_probeDeadline = 0;
WORD g_btnNow  = 0;
WORD g_btnLast = 0;
bool g_yHeld = false;
ULONGLONG g_dpadHoldSince[4] = {0, 0, 0, 0};
bool      g_dpadJumped[4]    = {false, false, false, false};

}  // namespace acc::pad
