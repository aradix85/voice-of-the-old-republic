# cycle_input_internal.h (71 lines)

Source: `patches/Accessibility/cycle_input_internal.h`

Internal to the cycle_input module (cycle_input*.cpp); no other file includes it. The public interface is cycle_input.h. Here: what the files share.

## Declarations (in source order)

- L48 `struct CategoryBindings`: Map a CycleCategory to its strings::Id pair (singular name + empty message).
- L56 `CategoryBindings BindingsFor( acc::filter::CycleCategory c, acc::filter::CycleContext ctx = acc::filter::CycleContext::World)`: cycle_input.cpp
- L59 `int FormatItemPayload(const char* name, bool haveYaw, int clock, int metres, char* outBuf, size_t outBufSize)`: cycle_input_announce.cpp
- L60 `void OnCycleItem(bool prev, acc::filter::CycleContext ctx)`
- L61 `void OnCycleEdge(bool last, acc::filter::CycleContext ctx)`
- L62 `void OnCycleCategory(bool prev, acc::filter::CycleContext ctx)`
- L65 `void OnAnnounceFocus()`: cycle_input_focus.cpp
- L66 `void OnPathfindFocus()`
- L67 `void OnRouteFocus()`
- L68 `void OnPathfindFocusForce()`

Also 1 constants or variables at namespace scope.
