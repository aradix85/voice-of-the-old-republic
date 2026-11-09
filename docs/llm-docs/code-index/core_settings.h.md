# core_settings.h (81 lines)

Source: `patches/Accessibility/core_settings.h`

Nav-system settings — minimal stub. Consumers go through Get() so the future user-options UI is a single-file swap. Behaviour-as-locked-design (bearing frame, cycle sort order, etc.) stays in consumer code.

## Declarations (in source order)

- L10 `struct Pillar1Settings`: Pillar 1 — small-scale change-driven cues.
- L31 `struct Pillar2Settings`: Pillar 2 — room/area announcements + view mode.
- L41 `struct Pillar3Settings`: Pillar 3 — guidance, map cursor, named markers.
- L54 `struct Pillar4Settings`: Pillar 4 — discrete object cycle.
- L65 `struct CrossPillarSettings`
- L70 `struct NavSettings`
- L78 `const NavSettings& Get()`
