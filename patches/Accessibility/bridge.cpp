// Local bridge — see bridge.h for the contract.
//
// Winsock must be included before <windows.h>.

#include "bridge_internal.h"

namespace acc::bridge {

namespace {

constexpr int   kFirstPort        = 47781;
constexpr int   kPortAttempts     = 10;
constexpr DWORD kSnapshotPeriodMs = 250;
constexpr DWORD kJobTimeoutMs     = 2000;
constexpr size_t kSpokenCapacity  = 300;

// ---------------------------------------------------------------------------
// Shared state between the socket thread and the main thread
struct Spoken {
    uint32_t    id;
    DWORD       ms;
    std::string text;  // UTF-8
};

std::mutex              g_mu;
std::condition_variable g_cv;
std::deque<Spoken>      g_spoken;
uint32_t                g_nextSpokenId = 1;
std::string             g_snapshot = "{\"ready\":false}";
DWORD                   g_snapshotMs = 0;
DWORD                   g_lastTickMs = 0;
std::deque<std::shared_ptr<Job>> g_jobs;

bool        g_started = false;
int         g_port = 0;
std::string g_token;

std::string RunJob(const Job& job) {
    if (job.path == "/nearby") {
        std::string r = Param(job, "radius");
        float radius = r.empty() ? 0.0f : static_cast<float>(std::atof(r.c_str()));
        return BuildNearby(radius);
    }
    if (job.path == "/do") return RunAction(job);
    if (job.path == "/overview") return BuildOverview();
    if (job.path == "/objects") return BuildObjects();
    if (job.path == "/rooms") return BuildRooms();
    if (job.path == "/menu") return BuildMenu();
    return "{\"error\":\"unknown job\"}";
}

void SendResponse(SOCKET c, int status, const std::string& body) {
    const char* reason = status == 200 ? "OK"
                       : status == 403 ? "Forbidden"
                       : status == 404 ? "Not Found"
                       : status == 504 ? "Gateway Timeout"
                       : "Error";
    std::string head = "HTTP/1.0 " + std::to_string(status) + " " + reason +
                       "\r\nContent-Type: application/json; charset=utf-8"
                       "\r\nCache-Control: no-store"
                       "\r\nContent-Length: " + std::to_string(body.size()) +
                       "\r\nConnection: close\r\n\r\n";
    std::string all = head + body;
    const char* p = all.data();
    int left = static_cast<int>(all.size());
    while (left > 0) {
        int sent = send(c, p, left, 0);
        if (sent <= 0) break;
        p += sent;
        left -= sent;
    }
}

std::string QueueAndWait(const std::string& path,
                         const std::map<std::string, std::string>& params,
                         int* status) {
    auto job = std::make_shared<Job>();
    job->path = path;
    job->params = params;
    std::unique_lock<std::mutex> lock(g_mu);
    g_jobs.push_back(job);
    bool done = g_cv.wait_for(lock, std::chrono::milliseconds(kJobTimeoutMs),
                              [&] { return job->done; });
    if (!done) {
        *status = 504;
        return "{\"error\":\"the game did not handle the request in time "
               "(is it minimised or loading?)\"}";
    }
    *status = 200;
    return job->result;
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
    size_t lineEnd = req.find("\r\n");
    if (lineEnd == std::string::npos) {
        SendResponse(c, 400, "{\"error\":\"bad request\"}");
        return;
    }
    std::string line = req.substr(0, lineEnd);
    size_t sp1 = line.find(' ');
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        SendResponse(c, 400, "{\"error\":\"bad request line\"}");
        return;
    }
    std::string target = line.substr(sp1 + 1, sp2 - sp1 - 1);

    // Token check (header names are case-insensitive).
    const std::string lower =
        acc::strfmt::ToLowerAscii(req.substr(0, req.find("\r\n\r\n")));
    const std::string want = acc::strfmt::ToLowerAscii("x-kotor-bridge: " + g_token);
    if (lower.find(want) == std::string::npos) {
        SendResponse(c, 403, "{\"error\":\"missing or wrong X-Kotor-Bridge token\"}");
        return;
    }

    std::string path = target, query;
    size_t qm = target.find('?');
    if (qm != std::string::npos) {
        path = target.substr(0, qm);
        query = target.substr(qm + 1);
    }
    std::map<std::string, std::string> params;
    ParseQuery(query, params);

    if (path == "/health") {
        DWORD now = GetTickCount();
        std::lock_guard<std::mutex> lock(g_mu);
        DWORD age = g_lastTickMs ? now - g_lastTickMs : 0xFFFFFFFF;
        SendResponse(c, 200, "{\"ok\":true,\"game\":" +
                     Json(acc::game::TitleName()) +
                     ",\"mod_version\":" +
                     Json(std::string(acc::kModVersion) + "+bridge") +
                     ",\"last_tick_ms_ago\":" + std::to_string(age) + "}");
        return;
    }
    if (path == "/spoken") {
        uint32_t since = static_cast<uint32_t>(
            std::strtoul(params["since"].c_str(), nullptr, 10));
        int max = params["max"].empty() ? 50 : std::atoi(params["max"].c_str());
        if (max <= 0 || max > 300) max = 50;
        std::lock_guard<std::mutex> lock(g_mu);
        std::vector<const Spoken*> picked;
        for (auto it = g_spoken.rbegin(); it != g_spoken.rend(); ++it) {
            if (it->id <= since) break;
            picked.push_back(&*it);
            if (static_cast<int>(picked.size()) >= max) break;
        }
        DWORD now = GetTickCount();
        std::string body = "{\"next\":" + std::to_string(g_nextSpokenId - 1) +
                           ",\"lines\":[";
        for (size_t i = picked.size(); i-- > 0;) {
            const Spoken* sp = picked[i];
            body += "{\"id\":" + std::to_string(sp->id) +
                    ",\"seconds_ago\":" + Num((now - sp->ms) / 1000.0) +
                    ",\"text\":" + Json(sp->text) + "}";
            if (i) body += ",";
        }
        body += "]}";
        SendResponse(c, 200, body);
        return;
    }
    if (path == "/state") {
        std::lock_guard<std::mutex> lock(g_mu);
        DWORD now = GetTickCount();
        std::string body = g_snapshot;
        // Splice the snapshot age into the object.
        if (!body.empty() && body.back() == '}') {
            body.pop_back();
            body += ",\"snapshot_ms_ago\":" +
                    std::to_string(g_snapshotMs ? now - g_snapshotMs : 0) + "}";
        }
        SendResponse(c, 200, body);
        return;
    }
    if (path == "/nearby" || path == "/do" || path == "/overview" ||
        path == "/menu" || path == "/objects" || path == "/rooms") {
        int status = 200;
        std::string body = QueueAndWait(path, params, &status);
        SendResponse(c, status, body);
        return;
    }
    SendResponse(c, 404, "{\"error\":\"unknown path\"}");
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

    DWORD now = GetTickCount();
    bool refresh = false;
    std::deque<std::shared_ptr<Job>> jobs;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        g_lastTickMs = now;
        refresh = (now - g_snapshotMs) >= kSnapshotPeriodMs || !g_snapshotMs;
        jobs.swap(g_jobs);
    }

    if (refresh) {
        std::string snap = BuildSnapshot();
        std::lock_guard<std::mutex> lock(g_mu);
        g_snapshot = std::move(snap);
        g_snapshotMs = now;
    }

    if (jobs.empty()) return;
    for (auto& job : jobs) {
        std::string result = RunJob(*job);
        acclog::Write("Bridge", "%s action=%s -> %.120s", job->path.c_str(),
                      Param(*job, "action").c_str(), result.c_str());
        std::lock_guard<std::mutex> lock(g_mu);
        job->result = std::move(result);
        job->done = true;
    }
    g_cv.notify_all();
}

}  // namespace acc::bridge
