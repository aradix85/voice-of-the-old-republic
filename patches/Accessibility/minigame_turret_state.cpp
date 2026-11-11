// The state the minigame_turret files share, defined in one place; each item is
// described where it is declared, in minigame_turret_internal.h.

#include "minigame_turret_internal.h"

namespace acc::turret_game {

State g_state;
acc::audio::LoopSource g_pegCue;
acc::audio::LoopSource g_humCue;

}  // namespace acc::turret_game
