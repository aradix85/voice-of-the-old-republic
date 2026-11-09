# menus_focus_k2.h (14 lines)

Source: `patches/Accessibility/menus_focus_k2.h`

KOTOR 2 focus announce — see menus_focus_k2.cpp for why this is a separate, deliberately minimal path rather than the KOTOR 1 handler.

## Declarations (in source order)

- L11 `void AnnounceFocus(void* panel, void* control, void* caller)`: Read the newly focused control's inline caption and speak it.
