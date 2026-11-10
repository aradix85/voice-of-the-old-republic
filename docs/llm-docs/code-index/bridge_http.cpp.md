# bridge_http.cpp (165 lines)

Source: `patches/Accessibility/bridge_http.cpp`

Local bridge: the HTTP and JSON text the server reads and writes.

## Declarations (in source order)

- L15 `int HexValue(char c)`
- L23 `std::string UrlDecode(const std::string& s)`: %XX and '+' decoded; a '%' without two hex digits after it stays.
- L46 `void ParseQuery(const std::string& q, Params& out)`
- L61 `bool IsBlank(char c)`
- L63 `const char* Reason(int status)`
- L76 `std::string Param(const Params& params, const char* key)`
- L81 `std::string Json(const std::string& s)`
- L106 `std::string Num(double v, int decimals)`
- L112 `std::string HandleJson(uint32_t handle)`
- L118 `bool ParseRequest(const std::string& request, std::string& path, Params& params)`
- L134 `bool HasToken(const std::string& request, const std::string& token)`
- L156 `std::string Response(int status, const std::string& body)`
