# tutorial_hints_internal.h (51 lines)

Source: `patches/Accessibility/tutorial_hints_internal.h`

Shared by the tutorial_hints files: strref texts resolved through the engine's TLK, so every match works in the game's own wording.

## Declarations (in source order)

- L18 `struct TlkTexts`: Up to Count strref texts of up to Len bytes, resolved on first use.
