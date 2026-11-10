# bridge.cpp (315 lines)

Source: `patches/Accessibility/bridge.cpp`

Local bridge, see bridge.h for the contract. This file: the socket thread, the job queue between it and the main thread, and Tick.

## Declarations (in source order)

- L57 `const Route* FindRoute(const std::string& path)`
- L90 `std::string Run(const Job& job)`
- L98 `void Send(SOCKET c, int status, const std::string& body)`
- L111 `std::string QueueAndWait(const Route& route, const Params& params, int& status)`: Hands the request to the main thread and waits for Tick to answer it.
- L127 `std::string Health()`
- L135 `std::string SpokenSince(const Params& params)`
- L159 `void HandleClient(SOCKET c)`
- L190 `void WriteConnectionFile()`
- L208 `std::string MakeToken()`
- L227 `void ServerThread(SOCKET listener)`
- L240 `void StartServer()`
- L282 `void RecordSpoken(const char* text)`
- L290 `void Tick()`
