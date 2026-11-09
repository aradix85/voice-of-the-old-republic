# bridge.cpp (1016 lines)

Source: `patches/Accessibility/bridge.cpp`

Local bridge — see bridge.h for the contract.

Winsock must be included before <windows.h>.

## Declarations (in source order)

- L67 `std::string ToUtf8(const char* text)`: Game codepage (what the engine and the speech path hand us) -> UTF-8.
- L84 `std::wstring Utf8ToWide(const std::string& s)`
- L95 `std::string Json(const std::string& s)`: Escape an UTF-8 string as a JSON string literal (with quotes).
- L120 `std::string JsonGame(const char* gameText)`
- L122 `std::string Num(double v, int decimals = 1)`
- L133 `float BearingDeg(float dx, float dy)`: Compass bearing (0 = north, clockwise) of the vector dx,dy in the engine frame (+X east, +Y north).
- L138 `const char* Dir8(float compassDeg)`
- L182 `PlayerFrame ReadPlayerFrame()`
- L191 `std::string SpokenName(void* obj)`: Name exactly as the in-world cycle speaks it (ordinal + state words), falling back to the plain engine name.
- L208 `std::string RelativeJson(const PlayerFrame& f, const Vector& p)`
- L224 `std::string CreatureJson(void* serverCreature)`
- L243 `std::string BuildSnapshot()`
- L389 `const char* CategoryKey(acc::filter::CycleCategory c)`
- L402 `std::string BuildNearby(float radius)`
- L443 `std::string BuildOverview()`
- L485 `std::string BuildObjects()`: Every door, loot container and ground item in the area with its live state, heard before or not, for the map tool (kotor_kaart.py --live) to lay over the static game files: which doors are open, which containers are e...
- L557 `std::string BuildRooms()`: The area's regions (the mod's room graph): kind, visited, reachable now, and which regions touch.
- L592 `std::string BuildMenu()`: The open screen as the mod's menu navigation sees it: every entry of the focus chain the arrow keys walk, with the focused one marked.
- L622 `uint32_t ParseHandle(const std::string& v)`
- L627 `std::string Param(const Job& job, const char* key)`
- L632 `std::string RunAction(const Job& job)`
- L689 `std::string RunJob(const Job& job)`
- L706 `std::string UrlDecode(const std::string& s)`
- L724 `void ParseQuery(const std::string& q, std::map<std::string, std::string>& out)`
- L739 `void SendResponse(SOCKET c, int status, const std::string& body)`
- L761 `std::string QueueAndWait(const std::string& path, const std::map<std::string, std::string>& params, int* status)`
- L780 `void HandleClient(SOCKET c)`
- L883 `void WriteConnectionFile()`
- L901 `std::string MakeToken()`
- L920 `void ServerThread(SOCKET listener)`
- L933 `void StartServer()`
- L975 `void RecordSpoken(const char* text)`
- L983 `void Tick()`
