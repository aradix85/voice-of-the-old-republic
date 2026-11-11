# bridge_describe.cpp (122 lines)

Source: `patches/Accessibility/bridge_describe.cpp`

Local bridge: how game things read in the JSON answers: game text as UTF-8, names as the mod speaks them, distance and direction from the player.

## Declarations (in source order)

- L28 `std::string GameFields()`
- L34 `std::string ToUtf8(const char* text)`: Game codepage (what the engine and the speech path hand us) -> UTF-8.
- L51 `std::wstring Utf8ToWide(const std::string& s)`
- L61 `std::string JsonGame(const char* gameText)`
- L63 `const char* Dir8(float compassDeg)`
- L67 `PlayerFrame ReadPlayerFrame()`
- L76 `std::string SpokenName(void* obj)`: Name exactly as the in-world cycle speaks it (ordinal + state words), falling back to the plain engine name.
- L93 `std::string RelativeJson(const PlayerFrame& f, const Vector& p)`
- L108 `const char* CategoryKey(acc::filter::CycleCategory c)`
