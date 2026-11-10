# view_mode_internal.h (55 lines)

Source: `patches/Accessibility/view_mode_internal.h`

Internal to the view_mode module (view_mode*.cpp); no other file includes it. The public interface is view_mode.h. Here: what the files share.

## Declarations (in source order)

- L23 `struct ViewModeState`
- L47 `void AnnounceCursorRegion(void* area, const Vector& cursor)`: view_mode_narrate.cpp
- L48 `void NarrateNearestObject(void* area, const Vector& cursor)`
- L51 `void PollEnter()`: view_mode_enter.cpp
- L52 `void ProcessPendingDispatch()`

Also 4 constants or variables at namespace scope.
