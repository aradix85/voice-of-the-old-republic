# pad_input_bindings.cpp (336 lines)

Source: `patches/Accessibility/pad_input_bindings.cpp`

Gamepad bindings both games share: routing a press into the mod's own overlays, the unified action menu, the trigger + face-button chord grid, the D-Pad in the world and on the map, and the shoulder chords.

## Declarations (in source order)

- L103 `bool OpenActionMenu()`: Open the unified action menu on whatever is focused — the pad's Shift+Enter.
- L130 `bool RouteToOverlay(int logical)`: Route a logical nav / activate / cancel code into whichever in-DLL overlay currently owns input.
- L163 `void ToggleActionMenu()`: LT — open the action menu, or close it if it is already up.
- L189 `bool ClaimQueueClearAll()`: The action queue's clear-all: a BARE Y while the queue is open.
- L196 `bool DispatchFaceChord(WORD button)`
- L264 `bool DispatchWorldDpad(int code)`: In-world D-Pad dispatch: the object cycle.
- L299 `bool DispatchMapDpad(int code)`: The map screen's D-Pad.
- L317 `bool DispatchShoulderChord(bool right)`: The shoulder chords: LT + LB speaks the route to the focused object, RT + RB walks to it.
