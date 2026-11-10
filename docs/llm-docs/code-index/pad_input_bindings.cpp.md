# pad_input_bindings.cpp (326 lines)

Source: `patches/Accessibility/pad_input_bindings.cpp`

Gamepad bindings both games share: routing a press into the mod's own overlays, the unified action menu, the trigger + face-button chord grid, the D-Pad in the world and on the map, and the shoulder chords.

## Declarations (in source order)

- L37 `bool OpenActionMenu()`: Open the unified action menu on whatever is focused — the pad's Shift+Enter.
- L64 `bool RouteToOverlay(int logical)`: Route a logical nav / activate / cancel code into whichever in-DLL overlay currently owns input.
- L97 `void ToggleActionMenu()`: LT — open the action menu, or close it if it is already up.
- L117 `bool ClaimQueueClearAll()`: The action queue's clear-all: a BARE Y while the queue is open.
- L129 `bool ClaimA()`: A: the overlay's activate, or in the world an interaction with what the player was last TOLD about, not the engine's last-clicked target (the promise keyboard Enter makes).
- L179 `bool DispatchFaceChord(WORD button)`: The face-button chord grid A trigger held turns each of the four face buttons into a second binding: LT + A announce the focused object (the minus key) LT + B open the action queue (Shift+H) LT + X your own status (H)...
- L244 `bool DispatchWorldDpad(int code)`: In-world D-Pad dispatch: the object cycle, left / right the object, up / down the category.
- L268 `bool DispatchMapDpad(int code)`: The map screen's D-Pad.
- L286 `bool DispatchShoulderChord(bool right)`: The shoulder chords: LT + LB speaks the route to the focused object, RT + RB walks to it.
- L311 `bool DispatchShoulderModeToggle(bool right)`: The bare shoulders on the container and the store, where the engine gives them nothing: the keyboard's Q / E mode toggle (take vs give, buy vs sell), without which a pad player could only ever take and only ever buy.
