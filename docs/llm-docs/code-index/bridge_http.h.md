# bridge_http.h (39 lines)

Source: `patches/Accessibility/bridge_http.h`

Local bridge: the HTTP and JSON text the server reads and writes. Engine-free and Windows-free, so tests/unit builds it.

## Declarations (in source order)

- L12 `using Params`
- L15 `std::string Param(const Params& params, const char* key)`: One query parameter, empty when absent.
- L18 `std::string Json(const std::string& s)`: An UTF-8 string as a JSON string literal, quotes included.
- L21 `std::string Num(double v, int decimals = 1)`: A number with a fixed count of decimals: "12.5".
- L24 `std::string HandleJson(uint32_t handle)`: An object handle as a JSON string: "0x0000003d".
- L29 `bool ParseRequest(const std::string& request, std::string& path, Params& params)`: The path and the decoded query parameters of a raw request ("GET /do?action=walk&x=1 HTTP/1.0\r\n...").
- L33 `bool HasToken(const std::string& request, const std::string& token)`: True when the request carries the header "X-Kotor-Bridge: ": the name in any case, the value exactly.
- L36 `std::string Response(int status, const std::string& body)`: A complete HTTP/1.0 response with a JSON body.
