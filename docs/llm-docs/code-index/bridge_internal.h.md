# bridge_internal.h (106 lines)

Source: `patches/Accessibility/bridge_internal.h`

Internal to the bridge module (bridge*.cpp); no other file includes it. The public interface is bridge.h. Here: what the files share.

## Declarations (in source order)

- L57 `struct Job`
- L67 `struct PlayerFrame`: Main-thread readers (all engine access goes through the mod's SEH-guarded helpers, so nothing here needs __try).
- L77 `std::string Json(const std::string& s)`: bridge_http.cpp
- L78 `std::string Num(double v, int decimals = 1)`
- L79 `void ParseQuery(const std::string& q, std::map<std::string, std::string>& out)`
- L82 `std::string ToUtf8(const char* text)`: bridge_describe.cpp
- L83 `std::wstring Utf8ToWide(const std::string& s)`
- L84 `std::string JsonGame(const char* gameText)`
- L85 `const char* Dir8(float compassDeg)`
- L86 `PlayerFrame ReadPlayerFrame()`
- L87 `std::string SpokenName(void* obj)`
- L88 `std::string RelativeJson(const PlayerFrame& f, const Vector& p)`
- L89 `const char* CategoryKey(acc::filter::CycleCategory c)`
- L92 `std::string BuildSnapshot()`: bridge_state.cpp
- L93 `std::string BuildMenu()`
- L96 `std::string BuildNearby(float radius)`: bridge_area.cpp
- L97 `std::string BuildOverview()`
- L98 `std::string BuildObjects()`
- L99 `std::string BuildRooms()`
- L102 `std::string Param(const Job& job, const char* key)`: bridge_actions.cpp
- L103 `std::string RunAction(const Job& job)`
