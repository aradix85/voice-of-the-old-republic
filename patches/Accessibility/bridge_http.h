// Local bridge: the HTTP and JSON text the server reads and writes.
// Engine-free and Windows-free, so tests/unit builds it.

#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace acc::bridge {

using Params = std::map<std::string, std::string>;

// One query parameter, empty when absent.
std::string Param(const Params& params, const char* key);

// An UTF-8 string as a JSON string literal, quotes included.
std::string Json(const std::string& s);

// A number with a fixed count of decimals: "12.5".
std::string Num(double v, int decimals = 1);

// An object handle as a JSON string: "0x0000003d".
std::string HandleJson(uint32_t handle);

// The path and the decoded query parameters of a raw request
// ("GET /do?action=walk&x=1 HTTP/1.0\r\n..."). False when the request line
// is malformed.
bool ParseRequest(const std::string& request, std::string& path, Params& params);

// True when the request carries the header "X-Kotor-Bridge: <token>": the
// name in any case, the value exactly.
bool HasToken(const std::string& request, const std::string& token);

// A complete HTTP/1.0 response with a JSON body.
std::string Response(int status, const std::string& body);

}  // namespace acc::bridge
