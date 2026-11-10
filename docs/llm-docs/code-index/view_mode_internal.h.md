# view_mode_internal.h (50 lines)

Source: `patches/Accessibility/view_mode_internal.h`

Internal to the view_mode module (view_mode*.cpp); no other file includes it. The public interface is view_mode.h. Here: what the files share.

## Declarations (in source order)

- L17 `struct ViewModeState`
- L42 `void AnnounceCursorRegion(void* area, const Vector& cursor)`: view_mode_narrate.cpp
- L43 `void NarrateNearestObject(void* area, const Vector& cursor)`
- L46 `void PollEnter()`: view_mode_enter.cpp
- L47 `void ProcessPendingDispatch()`

Also 2 constants or variables at namespace scope.
