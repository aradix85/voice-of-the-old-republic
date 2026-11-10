# bridge_internal.h (51 lines)

Source: `patches/Accessibility/bridge_internal.h`

Internal to the bridge module (bridge*.cpp); no other file includes it. The public interface is bridge.h. Here: what the main-thread readers share. All engine access goes through the mod's SEH-guarded helpers, so none of them needs __try.

## Declarations (in source order)

- L19 `struct PlayerFrame`
- L27 `std::string GameFields()`: bridge_describe.cpp
- L28 `std::string ToUtf8(const char* text)`
- L29 `std::wstring Utf8ToWide(const std::string& s)`
- L30 `std::string JsonGame(const char* gameText)`
- L31 `const char* Dir8(float compassDeg)`
- L32 `PlayerFrame ReadPlayerFrame()`
- L33 `std::string SpokenName(void* obj)`
- L34 `std::string RelativeJson(const PlayerFrame& f, const Vector& p)`
- L35 `const char* CategoryKey(acc::filter::CycleCategory c)`
- L38 `std::string BuildSnapshot()`: bridge_state.cpp
- L39 `std::string BuildMenu()`
- L42 `std::string BuildNearby(const Params& params)`: bridge_area.cpp
- L43 `std::string BuildOverview()`
- L44 `std::string BuildObjects()`
- L45 `std::string BuildRooms()`
- L48 `std::string RunAction(const Params& params)`: bridge_actions.cpp

Also 1 constants or variables at namespace scope.
