// Local bridge, see bridge.h for the contract. This file: the socket thread,
// the job queue between it and the main thread, and Tick.

// Winsock must be included before <windows.h>.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "bridge.h"
#include "bridge_internal.h"
#include "engine_game.h"
#include "log.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

namespace acc::bridge {

namespace {

constexpr int    kFirstPort      = 47781;
constexpr int    kPortAttempts   = 10;
constexpr DWORD  kJobTimeoutMs   = 2000;
constexpr size_t kSpokenCapacity = 300;

// The requests Tick answers on the main thread. k1Only: the engine reads
// behind them are mapped for KOTOR 1 only.
struct Route {
    const char* path;
    bool        k1Only;
    std::string (*run)(const Params&);
};

const Route kRoutes[] = {
    {"/state",    false, [](const Params&) { return BuildSnapshot(); }},
    {"/menu",     false, [](const Params&) { return BuildMenu(); }},
    {"/do",       false, RunAction},
    {"/nearby",   true,  BuildNearby},
    {"/overview", true,  [](const Params&) { return BuildOverview(); }},
    {"/objects",  true,  [](const Params&) { return BuildObjects(); }},
    {"/rooms",    true,  [](const Params&) { return BuildRooms(); }},
};

const Route* FindRoute(const std::string& path) {
    for (const Route& r : kRoutes) {
        if (path == r.path) return &r;
    }
    return nullptr;
}

struct Spoken {
    uint32_t    id;
    DWORD       ms;
    std::string text;  // UTF-8
};

struct Job {
    const Route* route = nullptr;
    Params       params;
    std::string  result;   // JSON body
    bool         done = false;
};

// Shared between the socket thread and the main thread, under g_mu.
std::mutex              g_mu;
std::condition_variable g_cv;
std::deque<Spoken>      g_spoken;
uint32_t                g_nextSpokenId = 1;
DWORD                   g_lastTickMs = 0;
std::deque<std::shared_ptr<Job>> g_jobs;

// Main thread only, before the socket thread starts.
bool        g_started = false;
int         g_port = 0;
std::string g_token;

std::string Run(const Job& job) {
    if (job.route->k1Only && !acc::game::IsKotor1()) {
        return "{\"error\":" +
               Json(std::string(job.route->path + 1) + " is KOTOR 1 only for now") + "}";
    }
    return job.route->run(job.params);
}

void Send(SOCKET c, int status, const std::string& body) {
    const std::string all = Response(status, body);
    const char* p = all.data();
    int left = static_cast<int>(all.size());
    while (left > 0) {
        int sent = send(c, p, left, 0);
        if (sent <= 0) break;
        p += sent;
        left -= sent;
    }
}

// Hands the request to the main thread and waits for Tick to answer it.
std::string QueueAndWait(const Route& route, const Params& params, int& status) {
    auto job = std::make_shared<Job>();
    job->route = &route;
    job->params = params;
    std::unique_lock<std::mutex> lock(g_mu);
    g_jobs.push_back(job);
    if (!g_cv.wait_for(lock, std::chrono::milliseconds(kJobTimeoutMs),
                       [&] { return job->done; })) {
        status = 504;
        return "{\"error\":\"the game did not handle the request in time "
               "(is it minimised or loading?)\"}";
    }
    status = 200;
    return job->result;
}

std::string Health() {
    const DWORD now = GetTickCount();
    std::lock_guard<std::mutex> lock(g_mu);
    const DWORD age = g_lastTickMs ? now - g_lastTickMs : 0xFFFFFFFF;
    return "{\"ok\":true," + GameFields() +
           ",\"last_tick_ms_ago\":" + std::to_string(age) + "}";
}

std::string SpokenSince(const Params& params) {
    const uint32_t since =
        static_cast<uint32_t>(std::strtoul(Param(params, "since").c_str(), nullptr, 10));
    int max = std::atoi(Param(params, "max").c_str());
    if (max <= 0 || max > static_cast<int>(kSpokenCapacity)) max = 50;
    std::lock_guard<std::mutex> lock(g_mu);
    std::vector<const Spoken*> picked;
    for (auto it = g_spoken.rbegin(); it != g_spoken.rend(); ++it) {
        if (it->id <= since) break;
        picked.push_back(&*it);
        if (static_cast<int>(picked.size()) >= max) break;
    }
    const DWORD now = GetTickCount();
    std::string body = "{\"next\":" + std::to_string(g_nextSpokenId - 1) + ",\"lines\":[";
    for (size_t i = picked.size(); i-- > 0;) {
        const Spoken* sp = picked[i];
        body += "{\"id\":" + std::to_string(sp->id) +
                ",\"seconds_ago\":" + Num((now - sp->ms) / 1000.0) +
                ",\"text\":" + Json(sp->text) + "}";
        if (i) body += ",";
    }
    return body + "]}";
}

void HandleClient(SOCKET c) {
    DWORD timeout = 3000;
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    std::string req;
    char buf[2048];
    while (req.find("\r\n\r\n") == std::string::npos && req.size() < 16384) {
        int got = recv(c, buf, sizeof(buf), 0);
        if (got <= 0) break;
        req.append(buf, static_cast<size_t>(got));
    }

    std::string path;
    Params params;
    if (!ParseRequest(req, path, params)) {
        Send(c, 400, "{\"error\":\"bad request\"}");
    } else if (!HasToken(req, g_token)) {
        Send(c, 403, "{\"error\":\"missing or wrong X-Kotor-Bridge token\"}");
    } else if (path == "/health") {
        Send(c, 200, Health());
    } else if (path == "/spoken") {
        Send(c, 200, SpokenSince(params));
    } else if (const Route* route = FindRoute(path)) {
        int status = 200;
        const std::string body = QueueAndWait(*route, params, status);
        Send(c, status, body);
    } else {
        Send(c, 404, "{\"error\":\"unknown path\"}");
    }
}

void WriteConnectionFile() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    char* slash = std::strrchr(exePath, '\\');
    if (!slash) return;
    strcpy_s(slash + 1, MAX_PATH - static_cast<size_t>(slash + 1 - exePath),
             "bridge.json");
    FILE* fp = nullptr;
    if (fopen_s(&fp, exePath, "wb") != 0 || !fp) {
        acclog::Write("Bridge", "could not write %s", exePath);
        return;
    }
    std::fprintf(fp, "{\"port\":%d,\"token\":\"%s\",\"pid\":%lu}\n", g_port,
                 g_token.c_str(), GetCurrentProcessId());
    fclose(fp);
    acclog::Write("Bridge", "connection file written: %s", exePath);
}

std::string MakeToken() {
    unsigned char bytes[16] = {0};
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        for (int i = 0; i < 16; ++i)
            bytes[i] = static_cast<unsigned char>(
                (q.QuadPart >> ((i % 8) * 8)) ^ (GetTickCount() >> i) ^ (i * 151));
    }
    std::string t;
    for (unsigned char b : bytes) {
        char buf[3];
        std::snprintf(buf, sizeof(buf), "%02x", b);
        t += buf;
    }
    return t;
}

void ServerThread(SOCKET listener) {
    for (;;) {
        SOCKET c = accept(listener, nullptr, nullptr);
        if (c == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }
        HandleClient(c);
        shutdown(c, SD_SEND);
        closesocket(c);
    }
}

void StartServer() {
    g_started = true;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        acclog::Write("Bridge", "WSAStartup failed");
        return;
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        acclog::Write("Bridge", "socket() failed: %d", WSAGetLastError());
        return;
    }
    // Refuse to share the port with another process.
    BOOL excl = TRUE;
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
               reinterpret_cast<const char*>(&excl), sizeof(excl));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int port = 0;
    for (int i = 0; i < kPortAttempts; ++i) {
        addr.sin_port = htons(static_cast<u_short>(kFirstPort + i));
        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            port = kFirstPort + i;
            break;
        }
    }
    if (!port || listen(s, 8) != 0) {
        acclog::Write("Bridge", "could not bind/listen on 127.0.0.1:%d..%d",
                      kFirstPort, kFirstPort + kPortAttempts - 1);
        closesocket(s);
        return;
    }
    g_port = port;
    g_token = MakeToken();
    WriteConnectionFile();
    std::thread(ServerThread, s).detach();
    acclog::Write("Bridge", "listening on http://127.0.0.1:%d/", port);
}

}  // namespace

void RecordSpoken(const char* text) {
    if (!text || !*text) return;
    std::string u = ToUtf8(text);
    std::lock_guard<std::mutex> lock(g_mu);
    g_spoken.push_back(Spoken{g_nextSpokenId++, GetTickCount(), std::move(u)});
    while (g_spoken.size() > kSpokenCapacity) g_spoken.pop_front();
}

void Tick() {
    if (!g_started) StartServer();

    std::deque<std::shared_ptr<Job>> jobs;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        g_lastTickMs = GetTickCount();
        jobs.swap(g_jobs);
    }
    if (jobs.empty()) return;

    for (auto& job : jobs) {
        std::string result = Run(*job);
        if (std::strcmp(job->route->path, "/do") == 0) {
            acclog::Write("Bridge", "/do action=%s -> %.120s",
                          Param(job->params, "action").c_str(), result.c_str());
        }
        std::lock_guard<std::mutex> lock(g_mu);
        job->result = std::move(result);
        job->done = true;
    }
    g_cv.notify_all();
}

}  // namespace acc::bridge
