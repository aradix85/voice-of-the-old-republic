# engine_player.cpp (162 lines)

Source: `patches/Accessibility/engine_player.cpp`

core player reads: position, facing, yaw, area and camera.

Party/leader resolution and the input-disable state machine were split out by the Phase-1 structure pass (refactoring candidate 5) into engine_player_party.cpp and engine_player_inputlock.cpp. This file also owns GetPlayerServerObject, the app-manager chain walk all three TUs read through - see engine_player_internal.h.

## Declarations (in source order)

- L21 `void* GetPlayerServerObject()`: Declared in engine_player_internal.h (with the chain-walk rationale) — no longer file-static because the party and input-lock TUs read it too.
- L49 `void* ClientToServerCreature(void* clientCreature)`
- L69 `bool GetPlayerPosition(Vector& out)`
- L82 `bool GetPlayerFacing(Vector& out)`
- L96 `bool GetPlayerYawDegrees(float& out)`
- L107 `void* GetPlayerArea()`
- L119 `bool GetCameraPosition(Vector& out)`
- L137 `bool GetCameraYawRadians(float& outRad)`
