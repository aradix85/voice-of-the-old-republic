// Internal to the interact files (interact_dispatch.cpp, interact_bare_keys.cpp)
// and input_poll_router.cpp, which calls them per tick. The public surface for
// other subsystems is interact_dispatch.h.

#pragma once

namespace acc::interact {

// interact_dispatch.cpp

// True when a bare interact key should first switch out of the in-game
// menu strip rather than dispatching.
bool ShouldSwitchFromInGameMenu();

// Enter / Shift+Enter interact on the current narrated target.
// forceRadial=true is the Shift+Enter (open the action menu) path.
void OnInteract(bool forceRadial);

// interact_bare_keys.cpp

// Bare 1-7: announce an empty target row / personal action-bar column.
void AnnounceBarePersonalKey(int slot);
void AnnounceBareTargetKey(int row);

}  // namespace acc::interact
