# core_settings.cpp (17 lines)

Source: `patches/Accessibility/core_settings.cpp`

(From core_settings.h.) Nav-system settings — minimal stub. Consumers go through Get() so the future user-options UI is a single-file swap. Behaviour-as-locked-design (bearing frame, cycle sort order, etc.) stays in consumer code.

## Declarations (in source order)

- L5 `const NavSettings& Get()`
