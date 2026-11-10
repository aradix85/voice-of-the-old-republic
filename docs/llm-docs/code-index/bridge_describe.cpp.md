# bridge_describe.cpp (114 lines)

Source: `patches/Accessibility/bridge_describe.cpp`

Local bridge: how game things read in the JSON answers (game text as UTF-8, names as the mod speaks them, distance and direction from the player).

## Declarations (in source order)

- L14 `float BearingDeg(float dx, float dy)`: Compass bearing (0 = north, clockwise) of the vector dx,dy in the engine frame (+X east, +Y north).
- L25 `std::string ToUtf8(const char* text)`: Game codepage (what the engine and the speech path hand us) -> UTF-8.
- L42 `std::wstring Utf8ToWide(const std::string& s)`
- L52 `std::string JsonGame(const char* gameText)`
- L54 `const char* Dir8(float compassDeg)`
- L58 `PlayerFrame ReadPlayerFrame()`
- L67 `std::string SpokenName(void* obj)`: Name exactly as the in-world cycle speaks it (ordinal + state words), falling back to the plain engine name.
- L84 `std::string RelativeJson(const PlayerFrame& f, const Vector& p)`
- L100 `const char* CategoryKey(acc::filter::CycleCategory c)`
