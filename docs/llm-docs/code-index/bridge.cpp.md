# bridge.cpp (304 lines)

Source: `patches/Accessibility/bridge.cpp`

Local bridge, see bridge.h for the contract. This file: the socket thread, the job queue between it and the main thread, and Tick.

## Declarations (in source order)

- L54 `const Route* FindRoute(const std::string& path)`
- L87 `void Send(SOCKET c, int status, const std::string& body)`
- L100 `std::string QueueAndWait(const Route& route, const Params& params, int& status)`: Hands the request to the main thread and waits for Tick to answer it.
- L116 `std::string Health()`
- L124 `std::string SpokenSince(const Params& params)`
- L148 `void HandleClient(SOCKET c)`
- L179 `void WriteConnectionFile()`
- L197 `std::string MakeToken()`
- L216 `void ServerThread(SOCKET listener)`
- L229 `void StartServer()`
- L271 `void RecordSpoken(const char* text)`
- L279 `void Tick()`
