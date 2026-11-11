# engine_area_passive.cpp (109 lines)

Source: `patches/Accessibility/engine_area_passive.cpp`

Keeps Q/E targeting and passive narration live while the engine holds a finished, obscuring fade (the Endar Spire opening).

## Declarations (in source order)

- L34 `bool MaybeDrivePassiveSelection()`: The Endar Spire opening HOLDS the global fade panel at alpha=1.0 for a long scripted stretch *after* the fade animation finishes (IsFadeDone), while the player still has full world control.
