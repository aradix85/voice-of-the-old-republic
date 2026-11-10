// Local bridge: the HTTP and JSON text the server reads and writes.

#include "bridge_http.h"

#include <cstdio>

#include "strfmt.h"

namespace acc::bridge {

namespace {

constexpr char kTokenHeader[] = "x-kotor-bridge:";

int HexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = acc::strfmt::AsciiLower(c);
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// %XX and '+' decoded; a '%' without two hex digits after it stays.
std::string UrlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '+') {
            out.push_back(' ');
            continue;
        }
        if (c == '%' && i + 2 < s.size()) {
            const int hi = HexValue(s[i + 1]);
            const int lo = HexValue(s[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>(hi * 16 + lo));
                i += 2;
                continue;
            }
        }
        out.push_back(c);
    }
    return out;
}

void ParseQuery(const std::string& q, Params& out) {
    size_t pos = 0;
    while (pos <= q.size()) {
        size_t amp = q.find('&', pos);
        if (amp == std::string::npos) amp = q.size();
        std::string pair = q.substr(pos, amp - pos);
        if (!pair.empty()) {
            size_t eq = pair.find('=');
            if (eq == std::string::npos) out[UrlDecode(pair)] = "";
            else out[UrlDecode(pair.substr(0, eq))] = UrlDecode(pair.substr(eq + 1));
        }
        pos = amp + 1;
    }
}

bool IsBlank(char c) { return c == ' ' || c == '\t'; }

const char* Reason(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 504: return "Gateway Timeout";
        default:  return "Error";
    }
}

}  // namespace

std::string Param(const Params& params, const char* key) {
    auto it = params.find(key);
    return it == params.end() ? std::string() : it->second;
}

std::string Json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    out.push_back('"');
    return out;
}

std::string Num(double v, int decimals) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, v);
    return buf;
}

std::string HandleJson(uint32_t handle) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08x", static_cast<unsigned>(handle));
    return Json(buf);
}

bool ParseRequest(const std::string& request, std::string& path, Params& params) {
    const size_t lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) return false;
    const std::string line = request.substr(0, lineEnd);
    const size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos) return false;
    const size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return false;
    const std::string target = line.substr(sp1 + 1, sp2 - sp1 - 1);
    const size_t qm = target.find('?');
    path = target.substr(0, qm);
    params.clear();
    if (qm != std::string::npos) ParseQuery(target.substr(qm + 1), params);
    return true;
}

bool HasToken(const std::string& request, const std::string& token) {
    if (token.empty()) return false;
    const size_t headersEnd = request.find("\r\n\r\n");
    size_t pos = request.find("\r\n");
    while (pos != std::string::npos && pos < headersEnd) {
        pos += 2;
        size_t eol = request.find("\r\n", pos);
        if (eol == std::string::npos) eol = request.size();
        const std::string line = request.substr(pos, eol - pos);
        const size_t nameLen = sizeof(kTokenHeader) - 1;
        if (acc::strfmt::ToLowerAscii(line.substr(0, nameLen)) == kTokenHeader) {
            size_t a = nameLen;
            size_t b = line.size();
            while (a < b && IsBlank(line[a])) ++a;
            while (b > a && IsBlank(line[b - 1])) --b;
            if (line.compare(a, b - a, token) == 0) return true;
        }
        pos = eol;
    }
    return false;
}

std::string Response(int status, const std::string& body) {
    return "HTTP/1.0 " + std::to_string(status) + " " + Reason(status) +
           "\r\nContent-Type: application/json; charset=utf-8"
           "\r\nCache-Control: no-store"
           "\r\nContent-Length: " + std::to_string(body.size()) +
           "\r\nConnection: close\r\n\r\n" + body;
}

}  // namespace acc::bridge
