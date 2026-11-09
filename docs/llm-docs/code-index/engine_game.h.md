# engine_game.h (101 lines)

Source: `patches/Accessibility/engine_game.h`

Which game and which build are we injected into?

Why this is its own module -------------------------- Two different questions used to be one. `engine_rebase` asked "is this the reference build of KOTOR 1, or the 2004 relink?" purely to decide how to map an address. With KOTOR 2 in scope there is a second, broader question — "which GAME is this?" — whose answers are consumed far outside address handling: feature gating for K1-only content (floor_puzzle, endar_softlock, tutorial_hints, ...), per-game struct offsets, and the input layer, where K2 has a native controller path K1 has no counterpart for.

Putting `IsKotor2()` in `acc::addr` would name a helper for the place it was first needed rather than for what it does. So identity lives here, and `engine_rebase` consumes it. There is exactly ONE detection mechanism.

How detection works ------------------- The PE link timestamp of the *game's* image (not this DLL's). Reading it is one header walk of already-mapped memory: no file I/O, no allocation, no dependency on another global's initialisation. That matters because the engine-address constants that consume this are themselves dynamically initialised at load time, in unspecified order across translation units — the same constraint that shaped `engine_rebase`.

Hashing the file would be more precise and is what the KPatchManager version gate already does before we are ever loaded. By the time this runs, the build has been vetted; all we need is to tell the vetted builds apart.

Adding a build -------------- Add the timestamp constant and its `Build` enumerator, map it to a `Title`, and give it a name. Anything not listed resolves to `Title::Unknown` / `Build::Unknown`, which callers must treat as "do nothing engine-specific" — see the failure-mode note on `acc::addr::R`.

## Declarations (in source order)

- L45 `enum class Title`: The game itself.
- L55 `enum class Build`: A specific executable build.
- L64 `Title CurrentTitle()`: Both are resolved once and cached for the process lifetime.
- L65 `Build CurrentBuild()`
- L70 `inline bool IsKotor1()`: The two questions almost every caller actually has.
- L71 `inline bool IsKotor2()`
- L76 `const char* TitleName()`: Short, stable names for logs: "kotor1" / "kotor2" / "unknown", and "reference" / "relink-2004-03-05" / "aspyr-2015" / "relink-2021-09-09" / "unknown".
- L77 `const char* BuildName()`
- L98 `inline bool HandlerEnabled()`: Default-deny gate for hook handlers during the KOTOR 2 port.
