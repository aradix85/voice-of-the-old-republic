# pad_input.cpp (2020 lines)

Source: `patches/Accessibility/pad_input.cpp`

Gamepad input — see pad_input.h for why this is one module and not a pad case scattered through every handler, and for why KOTOR 1 and KOTOR 2 share every binding but not their event source.

## Declarations (in source order)

- L236 `void NoteEnginePadLive(const char* why)`
- L256 `void NoteSeen()`: Only the two engine-native-discriminated event shapes are allowed to set this: a stick axis carrying val = +/-1 (the engine's own preamble keys on exactly that), and a D-Pad code (F9..F12, unbound in stock [Keymapping...
- L264 `void EnsureXInput()`
- L328 `bool PhysDown(int vk)`: Pad-vs-keyboard discrimination
- L339 `int KeyboardTwinVk(int code)`: The keyboard row each shared code belongs to.
- L376 `bool InWorldSurface()`: Surfaces "World" means the pad is driving the game, not a panel: the player exists and no engine panel owns the foreground.
- L399 `void NoteNavPress()`: The cancel a pad press has no keystroke to trigger.
- L408 `bool RouteToOverlay(int logical)`: Route a logical nav / activate / cancel code into whichever in-DLL overlay currently owns input.
- L457 `bool OpenActionMenu()`: Open the unified action menu on whatever is focused — the pad's Shift+Enter.
- L487 `void ToggleActionMenu()`: LT — open the action menu, or close it if it is already up.
- L513 `bool ClaimQueueClearAll()`: The action queue's clear-all: a BARE Y while the queue is open.
- L578 `bool DispatchFaceChord(WORD button)`
- L646 `bool DispatchWorldDpad(int code)`: In-world D-Pad dispatch: the object cycle.
- L681 `bool DispatchMapDpad(int code)`: The map screen's D-Pad.
- L737 `void FireGameAction(int actionId, int defaultDik, const char* what)`: Fire a [Keymapping] game action by synthesising the key it is bound to.
- L755 `void InjectMenuKey(int logical)`: Navigate an ENGINE panel: send the arrow / Enter / Esc the keyboard would have sent.
- L771 `int LogicalForDirection(int code)`: The logical code a pad D-Pad / stick direction stands in for.
- L789 `void DispatchDpad(int code)`
- L812 `void DispatchA()`
- L832 `void DispatchB()`
- L855 `bool DispatchShoulderChord(bool right)`: The shoulder chords: LT + LB beacons to the focused object, RT + RB walks to it.
- L873 `void DispatchShoulder(bool right)`
- L958 `int DpadSlotForCode(int code)`
- L970 `bool DpadHoldSpent(int code)`: True while this direction's hold has already spent its jump.
- L977 `void DispatchDpadHold(int code)`: Pay the hold.
- L1030 `bool AdoptLivelierSlot(ULONGLONG now)`: The idle-slot guard — see kLatchIdleMs for what it is defending against.
- L1131 `bool EnginePadDead()`
- L1133 `bool LeftTriggerHeld()`
- L1134 `bool RightTriggerHeld()`
- L1135 `bool Connected()`
- L1141 `void Tick()`
- L1352 `bool IsPadCode(int code)`
- L1368 `const char* CodeHint(int code)`
- L1388 `bool IsPhysicalF1()`
- L1393 `bool StickMoving()`
- L1395 `bool InWorld()`
- L1397 `bool StickVector(float& outX, float& outY)`
- L1404 `bool RightStickVector(float& outX, float& outY)`
- L1411 `void PollButtons()`
- L1535 `bool PeekModifierHeld()`
- L1537 `void PollXInputButtons()`
- L1676 `void PollTriggers()`
- L1720 `bool TranslateClientEvent(int code, int value)`
- L1757 `Verdict TranslateManagerEvent(int& code, int& value)`
