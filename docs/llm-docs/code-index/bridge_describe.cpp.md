# bridge_describe.cpp (129 lines)

Source: `patches/Accessibility/bridge_describe.cpp`

Local bridge: how game things read in the JSON answers: game text as UTF-8, names as the mod speaks them, distance and direction from the player.

## Declarations (in source order)

- L28 `float BearingDeg(float dx, float dy)`: Compass bearing (0 = north, clockwise) of the vector dx,dy in the engine frame (+X east, +Y north).
- L35 `std::string GameFields()`
- L41 `std::string ToUtf8(const char* text)`: Game codepage (what the engine and the speech path hand us) -> UTF-8.
- L58 `std::wstring Utf8ToWide(const std::string& s)`
- L68 `std::string JsonGame(const char* gameText)`
- L70 `const char* Dir8(float compassDeg)`
- L74 `PlayerFrame ReadPlayerFrame()`
- L83 `std::string SpokenName(void* obj)`: Name exactly as the in-world cycle speaks it (ordinal + state words), falling back to the plain engine name.
- L100 `std::string RelativeJson(const PlayerFrame& f, const Vector& p)`
- L115 `const char* CategoryKey(acc::filter::CycleCategory c)`
