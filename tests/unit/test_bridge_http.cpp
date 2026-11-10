// bridge_http: the request parsing, token check and JSON text of the local
// bridge.

#include <string>

#include "bridge_http.h"
#include "doctest.h"

using namespace acc::bridge;

TEST_CASE("Json escapes quotes, backslashes and control characters") {
    CHECK(Json("Door") == "\"Door\"");
    CHECK(Json("say \"hi\"\\") == "\"say \\\"hi\\\"\\\\\"");
    CHECK(Json("a\nb\tc") == "\"a\\nb\\tc\"");
    CHECK(Json(std::string("\x01", 1)) == "\"\\u0001\"");
    CHECK(Json("Matrik\xE2\x80\x99s") == "\"Matrik\xE2\x80\x99s\"");  // UTF-8 passes
}

TEST_CASE("Num and HandleJson") {
    CHECK(Num(12.345) == "12.3");
    CHECK(Num(271.6, 0) == "272");
    CHECK(HandleJson(0x3d) == "\"0x0000003d\"");
    CHECK(HandleJson(0x7ffffffd) == "\"0x7ffffffd\"");
}

TEST_CASE("ParseRequest splits the path and decodes the query") {
    std::string path;
    Params p;
    REQUIRE(ParseRequest("GET /do?action=say&text=Hallo+daar%21 HTTP/1.0\r\n\r\n", path, p));
    CHECK(path == "/do");
    CHECK(Param(p, "action") == "say");
    CHECK(Param(p, "text") == "Hallo daar!");
    CHECK(Param(p, "missing").empty());

    REQUIRE(ParseRequest("GET /state HTTP/1.1\r\nHost: x\r\n\r\n", path, p));
    CHECK(path == "/state");
    CHECK(p.empty());

    REQUIRE(ParseRequest("GET /do?text=100%&flag HTTP/1.0\r\n", path, p));
    CHECK(Param(p, "text") == "100%");   // a bare % stays
    CHECK(p.count("flag") == 1);

    CHECK_FALSE(ParseRequest("GET /state", path, p));          // no line end
    CHECK_FALSE(ParseRequest("GARBAGE\r\n\r\n", path, p));
}

TEST_CASE("HasToken wants the header with exactly the token") {
    const std::string token = "0123abcd";
    CHECK(HasToken("GET / HTTP/1.0\r\nX-Kotor-Bridge: 0123abcd\r\n\r\n", token));
    CHECK(HasToken("GET / HTTP/1.0\r\nHost: a\r\nx-kotor-bridge:0123abcd  \r\n\r\n", token));
    CHECK_FALSE(HasToken("GET / HTTP/1.0\r\nX-Kotor-Bridge: 0123abcdXX\r\n\r\n", token));
    CHECK_FALSE(HasToken("GET / HTTP/1.0\r\nX-Kotor-Bridge: wrong\r\n\r\n", token));
    CHECK_FALSE(HasToken("GET / HTTP/1.0\r\n\r\nX-Kotor-Bridge: 0123abcd\r\n", token));  // body
    CHECK_FALSE(HasToken("GET /?X-Kotor-Bridge: 0123abcd HTTP/1.0\r\n\r\n", token));    // request line
    CHECK_FALSE(HasToken("GET / HTTP/1.0\r\nX-Kotor-Bridge: \r\n\r\n", ""));
}

TEST_CASE("Response carries status, length and the body") {
    const std::string r = Response(404, "{}");
    CHECK(r.rfind("HTTP/1.0 404 Not Found\r\n", 0) == 0);
    CHECK(r.find("Content-Length: 2\r\n") != std::string::npos);
    CHECK(r.size() >= 6);
    CHECK(r.substr(r.size() - 6) == "\r\n\r\n{}");
    CHECK(Response(400, "").rfind("HTTP/1.0 400 Bad Request", 0) == 0);
}
