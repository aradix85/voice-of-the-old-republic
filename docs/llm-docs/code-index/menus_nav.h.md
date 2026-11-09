# menus_nav.h (38 lines)

Source: `patches/Accessibility/menus_nav.h`

Cursor stepping shared by the standalone screen navigators.

Header-only: one integer function with no engine access, so it needs no TU of its own. Lives here rather than in menus_internal.h, which declares itself a private contract between menus.cpp and menus_extract.cpp.

## Declarations (in source order)

- L24 `inline int ClampedCursorStep(int cur, int count, bool up, bool down, bool home, bool end)`: Step `cur` by one (Up/Down) or jump to an end (Home/End), clamped to [0, count-1].
