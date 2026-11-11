# engine_game.cpp (57 lines)

Source: `patches/Accessibility/engine_game.cpp`

(From engine_game.h.) Which game are we injected into?

The mod supports one executable per game: KOTOR 1 version 1.0.3 (Steam, GOG and the GOG-equivalent CD re-pack are the same code) and KOTOR 2 as Aspyr rebuilt it in 2015 (Steam). The patch manager's version gate has checked the executable's hash before this DLL is loaded; here the two only have to be told apart.

That is done by the PE link timestamp of the game's image (not this DLL's): one header walk of mapped memory, no file I/O, no allocation, no dependency on another global's initialisation. It has to be that cheap because the engine-address constants that ask are themselves initialised at load time, in unspecified order across translation units.

## Declarations (in source order)

- L15 `uint32_t ReadGameLinkTimestamp()`
- L31 `Title DetectTitle()`
- L43 `Title CurrentTitle()`: Function-local static: initialised on first use, with no dependency on any other global's initialisation order.
- L48 `const char* TitleName()`
