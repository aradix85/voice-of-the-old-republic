# engine_game.h (54 lines)

Source: `patches/Accessibility/engine_game.h`

Which game are we injected into?

The mod supports one executable per game: KOTOR 1 version 1.0.3 (Steam, GOG and the GOG-equivalent CD re-pack are the same code) and KOTOR 2 as Aspyr rebuilt it in 2015 (Steam). The patch manager's version gate has checked the executable's hash before this DLL is loaded; here the two only have to be told apart.

That is done by the PE link timestamp of the game's image (not this DLL's): one header walk of mapped memory, no file I/O, no allocation, no dependency on another global's initialisation. It has to be that cheap because the engine-address constants that ask are themselves initialised at load time, in unspecified order across translation units.

## Declarations (in source order)

- L19 `enum class Title`
- L26 `Title CurrentTitle()`: Resolved once and cached for the process lifetime.
- L30 `inline bool IsKotor1()`: An unknown executable is neither, so `!IsKotor1()` is NOT `IsKotor2()`: gate on the one you mean.
- L31 `inline bool IsKotor2()`
- L34 `const char* TitleName()`: For logs: "kotor1", "kotor2" or "unknown".
- L51 `inline bool HandlerEnabled()`: Default-deny gate for hook handlers during the KOTOR 2 port.
