# bridge_http.cpp (80 lines)

Source: `patches/Accessibility/bridge_http.cpp`

Local bridge: HTTP and JSON text helpers. Engine-free, so the unit tests build them.

## Declarations (in source order)

- L12 `std::string UrlDecode(const std::string& s)`
- L33 `std::string Json(const std::string& s)`: Escape an UTF-8 string as a JSON string literal (with quotes).
- L58 `std::string Num(double v, int decimals)`
- L64 `void ParseQuery(const std::string& q, std::map<std::string, std::string>& out)`
