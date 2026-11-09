# pad_input.cpp (2015 lines)

Source: `patches/Accessibility/pad_input.cpp`

Gamepad input — see pad_input.h for why this is one module and not a pad case scattered through every handler, and for why KOTOR 1 and KOTOR 2 share every binding but not their event source.

## Declarations (in source order)

- L235 `void NoteEnginePadLive(const char* why)`
- L255 `void NoteSeen()`: Only the two engine-native-discriminated event shapes are allowed to set this: a stick axis carrying val = +/-1 (the engine's own preamble keys on exactly that), and a D-Pad code (F9..F12, unbound in stock [Keymapping...
- L263 `void EnsureXInput()`
- L327 `bool PhysDown(int vk)`: Pad-vs-keyboard discrimination
- L338 `int KeyboardTwinVk(int code)`: The keyboard row each shared code belongs to.
- L375 `bool InWorldSurface()`: Surfaces "World" means the pad is driving the game, not a panel: the player exists and no engine panel owns the foreground.
- L395 `void NoteNavPress()`: The cancel a pad press has no keystroke to trigger.
- L403 `bool RouteToOverlay(int logical)`: Route a logical nav / activate / cancel code into whichever in-DLL overlay currently owns input.
- L452 `bool OpenActionMenu()`: Open the unified action menu on whatever is focused — the pad's Shift+Enter.
- L482 `void ToggleActionMenu()`: LT — open the action menu, or close it if it is already up.
- L508 `bool ClaimQueueClearAll()`: The action queue's clear-all: a BARE Y while the queue is open.
- L573 `bool DispatchFaceChord(WORD button)`
- L641 `bool DispatchWorldDpad(int code)`: In-world D-Pad dispatch: the object cycle.
- L676 `bool DispatchMapDpad(int code)`: The map screen's D-Pad.
- L732 `void FireGameAction(int actionId, int defaultDik, const char* what)`: Fire a [Keymapping] game action by synthesising the key it is bound to.
- L750 `void InjectMenuKey(int logical)`: Navigate an ENGINE panel: send the arrow / Enter / Esc the keyboard would have sent.
- L766 `int LogicalForDirection(int code)`: The logical code a pad D-Pad / stick direction stands in for.
- L784 `void DispatchDpad(int code)`
- L807 `void DispatchA()`
- L827 `void DispatchB()`
- L850 `bool DispatchShoulderChord(bool right)`: The shoulder chords: LT + LB beacons to the focused object, RT + RB walks to it.
- L868 `void DispatchShoulder(bool right)`
- L953 `int DpadSlotForCode(int code)`
- L965 `bool DpadHoldSpent(int code)`: True while this direction's hold has already spent its jump.
- L972 `void DispatchDpadHold(int code)`: Pay the hold.
- L1025 `bool AdoptLivelierSlot(ULONGLONG now)`: The idle-slot guard — see kLatchIdleMs for what it is defending against.
- L1126 `bool EnginePadDead()`
- L1128 `bool LeftTriggerHeld()`
- L1129 `bool RightTriggerHeld()`
- L1130 `bool Connected()`
- L1136 `void Tick()`
- L1347 `bool IsPadCode(int code)`
- L1363 `const char* CodeHint(int code)`
- L1383 `bool IsPhysicalF1()`
- L1388 `bool StickMoving()`
- L1390 `bool InWorld()`
- L1392 `bool StickVector(float& outX, float& outY)`
- L1399 `bool RightStickVector(float& outX, float& outY)`
- L1406 `void PollButtons()`
- L1530 `bool PeekModifierHeld()`
- L1532 `void PollXInputButtons()`
- L1671 `void PollTriggers()`
- L1715 `bool TranslateClientEvent(int code, int value)`
- L1752 `Verdict TranslateManagerEvent(int& code, int& value)`
