# pad_input_internal.h (190 lines)

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
- L172 `bool ClaimA()`
- L173 `bool DispatchFaceChord(WORD button)`
- L174 `bool DispatchWorldDpad(int code)`
- L175 `bool DispatchMapDpad(int code)`
- L176 `bool DispatchShoulderChord(bool right)`
- L177 `bool DispatchShoulderModeToggle(bool right)`
- L180 `void FireGameAction(int actionId, int defaultDik, const char* what)`: pad_input_dispatch.cpp
- L181 `int LogicalForDirection(int code)`
- L182 `void DispatchDpad(int code)`
- L183 `void DispatchA()`
- L184 `void DispatchB()`
- L185 `void DispatchShoulder(bool right)`
- L186 `bool DpadHoldSpent(int code)`
- L187 `void DispatchDpadHold(int code)`

Also 52 constants or variables at namespace scope.
