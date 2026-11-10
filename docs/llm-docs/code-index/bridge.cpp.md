# bridge.cpp (329 lines)

Source: `patches/Accessibility/bridge.cpp`

Local bridge — see bridge.h for the contract.

Winsock must be included before <windows.h>.

## Declarations (in source order)

- L38 `std::string RunJob(const Job& job)`
- L52 `void SendResponse(SOCKET c, int status, const std::string& body)`
- L74 `std::string QueueAndWait(const std::string& path, const std::map<std::string, std::string>& params, int* status)`
- L93 `void HandleClient(SOCKET c)`
- L196 `void WriteConnectionFile()`
- L214 `std::string MakeToken()`
- L233 `void ServerThread(SOCKET listener)`
- L246 `void StartServer()`
- L288 `void RecordSpoken(const char* text)`
- L296 `void Tick()`
