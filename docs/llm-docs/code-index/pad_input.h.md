# pad_input.h (360 lines)

Source: `patches/Accessibility/pad_input.h`

Gamepad input — the one place that knows what a pad event looks like.

Two games, two event SOURCES, one meaning ----------------------------------------- KOTOR 2's Steam/Aspyr build ships real gamepad support: its DirectInput layer enumerates game controllers and delivers pad presses to the engine's own input path, so on K2 the mod's job is to TRANSLATE events it is already being handed (TranslateManagerEvent / TranslateClientEvent).

KOTOR 1 hands us nothing. Its raw-input layer creates exactly two DirectInput devices — the system keyboard and the system mouse — and no other code in the executable ever creates a third. The Aurora joystick plumbing is all still there (`CExoRawInputInternal::GetJoystickBuffer` is a near-twin of K2's reader, and the InputIndices enum still has JOYSTICK_XAXIS / JOYSTICK_BUTTON0 ..14) but the device array is zeroed in the constructor and written by nothing, so the read early-outs before it touches anything. There is no engine path to detour into life. On K1 the mod therefore IS the pad driver: it reads the pad through XInput and pushes the result back in.

What that changes, and what it does not:

* The MEANING of every pad control is shared. Both games run the same helpers — RouteToOverlay, DispatchWorldDpad, DispatchMapDpad, InteractNarratedTarget, the trigger layer — so the two play identically and a binding is never written twice. * Only the SOURCE differs. K2: engine events arriving at two hooks. K1: PollButtons(), edge-detecting the XInput button word. * Where the ENGINE has to act (walk, turn the camera, navigate one of its own panels, fire a game action), K1 synthesises the player's bound key as a DirectInput scancode — key_inject, the mechanism camera_orient's snap-turn has always used. The pad in a KOTOR 1 menu is therefore literally the keyboard, which is exactly what it is on K2.

Why this module exists ---------------------- KOTOR 2's Steam/Aspyr build ships working gamepad support, and it delivers pad events through the SAME engine input path (and the same InputIndex numbering) as the keyboard. `InputIndexName` therefore prints misleading keyboard names for pad events, and — worse — every downstream handler in the mod is written against the keyboard's logical codes (kInputNav*, kInputEnter*, kInputEsc*). Scattering "…or the pad code" into each of those handlers would put one piece of knowledge in a dozen places.

So there is exactly ONE seam: TranslateManagerEvent, called as the first statement of OnHandleInputEvent. It normalises a pad event into the logical code the rest of the mod already understands, or handles it outright when the pad has a binding the keyboard does not. Nothing downstream of that call knows a controller exists.

The two Translate* seams stay KOTOR 2 only: no pad event can ever arrive at those hooks on KOTOR 1, which also means the shared keyboard codes (F9..F12, A, B, C, D) can never be misread there — the whole twin-key discrimination layer is K2's problem alone.

Engine reference: docs/llm-docs/k2-controller-support.md. Work plan + test steps: docs/kotor2-controller-plan.md, docs/kotor1-controller-port.md.

## Declarations (in source order)

- L122 `enum class Verdict`: What the caller must do with the event after the seam has looked at it.
- L140 `Verdict TranslateManagerEvent(int& code, int& value)`: Normalise one CSWGuiManager::HandleInputEvent event.
- L150 `bool TranslateClientEvent(int code, int value)`: The same seam for the in-world client route (OnClientHandleInputEvent).
- L155 `bool IsPadCode(int code)`: True iff `code` could be a pad event on the running build.
- L165 `const char* CodeHint(int code)`: Short label for what a pad-plausible code means on the pad ("D-Pad down", "A", "left stick vertical"), or nullptr when the code cannot be a pad event on the running build.
- L170 `bool IsPhysicalF1()`: True iff a raw 0x27 arriving at the GUI manager is a PHYSICAL F1 rather than the pad's A button.
- L234 `void Tick()`: Per-tick SAMPLE: refreshes trigger state, stick vectors, the button word and pad presence, and edge-detects the trigger layer (whose jobs PollTriggers then runs).
- L246 `bool EnginePadDead()`: True once the mod has to drive the pad itself, because the ENGINE's own gamepad path is not delivering.
- L259 `void PollButtons()`: Edge-detect the button word Tick() sampled and dispatch it.
- L266 `void PollTriggers()`: Both games: run the trigger layer's solo jobs, which Tick() only claimed.
- L284 `void PollXInputButtons()`: The buttons the mod reads from XInput itself X held with a trigger is the pad's H key: LT + X is the self-status readout (the keyboard's H), RT + X is the action queue (Shift+H).
- L292 `bool PeekModifierHeld()`: True while the pad's Y button is held.
- L294 `bool LeftTriggerHeld()`
- L295 `bool RightTriggerHeld()`
- L310 `bool Connected()`: True while a gamepad is present.
- L331 `bool StickMoving()`: Analog movement (Phase 2) The engine's keyboard-only "is the player commanding translation" query (engine_keymap::ForwardBackwardKeyHeld) reads VKs, so a stick-driven walk holds no key and never satisfies it — which i...
- L341 `bool InWorld()`: "The pad is driving the game, not a panel": a player exists and no engine panel owns the foreground.
- L352 `bool StickVector(float& outX, float& outY)`: The left stick as a normalised direction, screen convention: x = +1 right, y = +1 UP (the axis as XInput reports it, not the map's inverted pixel space — the caller flips if it wants pixels).
- L357 `bool RightStickVector(float& outX, float& outY)`: The RIGHT stick, same convention and same sample.

Also 18 constants or variables at namespace scope.
