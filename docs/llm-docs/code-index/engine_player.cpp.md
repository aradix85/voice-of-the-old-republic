# engine_player.cpp (138 lines)

Source: `patches/Accessibility/engine_player.cpp`

core player reads: position, facing, yaw, area and camera.

Party/leader resolution and the input-disable state machine were split out by the Phase-1 structure pass (refactoring candidate 5) into engine_player_party.cpp and engine_player_inputlock.cpp. This file also owns GetPlayerServerObject, the app-manager chain walk all three TUs read through - see engine_player_internal.h.

## Declarations (in source order)

- L23 `void* GetPlayerServerObject()`: Declared in engine_player_internal.h (with the chain-walk rationale) — no longer file-static because the party and input-lock TUs read it too.
- L51 `void* ClientToServerCreature(void* clientCreature)`
- L71 `bool GetPlayerPosition(Vector& out)`
- L76 `bool GetPlayerFacing(Vector& out)`
- L83 `bool GetPlayerYawDegrees(float& out)`
- L91 `void* GetPlayerArea()`
- L103 `bool GetCameraPosition(Vector& out)`
- L113 `bool GetCameraYawRadians(float& outRad)`
