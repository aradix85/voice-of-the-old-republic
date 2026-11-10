# pad_input_internal.h (188 lines)

Source: `patches/Accessibility/pad_input_internal.h`

Internal to the pad_input module (pad_input*.cpp); no other file includes it. The public interface is pad_input.h. Here: what the files share.

## Declarations (in source order)

- L95 `enum class EnginePad`: Is the ENGINE's own gamepad path alive?
- L164 `void NoteSeen()`: pad_input.cpp
- L165 `bool InWorldSurface()`
- L166 `void NoteNavPress()`
- L169 `bool RouteToOverlay(int logical)`: pad_input_bindings.cpp
- L170 `void ToggleActionMenu()`
- L171 `bool ClaimQueueClearAll()`
- L172 `bool DispatchFaceChord(WORD button)`
- L173 `bool DispatchWorldDpad(int code)`
- L174 `bool DispatchMapDpad(int code)`
- L175 `bool DispatchShoulderChord(bool right)`
- L178 `void FireGameAction(int actionId, int defaultDik, const char* what)`: pad_input_dispatch.cpp
- L179 `int LogicalForDirection(int code)`
- L180 `void DispatchDpad(int code)`
- L181 `void DispatchA()`
- L182 `void DispatchB()`
- L183 `void DispatchShoulder(bool right)`
- L184 `bool DpadHoldSpent(int code)`
- L185 `void DispatchDpadHold(int code)`

Also 52 constants or variables at namespace scope.
