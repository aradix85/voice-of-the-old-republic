#include "log.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <share.h>

namespace acclog {

namespace {

char  g_logDir[MAX_PATH]   = "logs";
char  g_logPath[MAX_PATH]  = "logs\\patch.log";
char  g_patchDir[MAX_PATH] = "";
FILE* g_logFile            = nullptr;
bool  g_logOpenAttempted   = false;

ULONGLONG g_bringupBaselineMs = 0;

// Hooks can fire from other threads than the main one (audio dispatch, async
// loaders). One lock guards the file handle and all dedup state.
CRITICAL_SECTION g_lock;
bool             g_lockInited = false;

struct Locker {
    Locker()  { if (g_lockInited) EnterCriticalSection(&g_lock); }
    ~Locker() { if (g_lockInited) LeaveCriticalSection(&g_lock); }
};

constexpr size_t kMessageBuf = 1024;
constexpr int    kMaxKeys    = 96;
// A pending count is written out once its tag has been quiet this long.
constexpr DWORD  kStaleMs    = 1000;

// Per-tag state of Trace, Once and Edge. Tags are matched by content, so a
// literal and a formatted key both work.
struct DedupEntry {
    char  tag[64];
    char  lastContent[kMessageBuf];
    int   suppressed;     // Trace: identical fires since lastContent was written
    int   state;          // Edge: the last reported state
    int   stateHeld;      // Edge: fires in that state since it was reported
    bool  hasContent;     // Trace has written for this tag
    bool  hasState;       // Edge has written for this tag
    bool  isOnce;         // Once has written for this tag
    DWORD lastSeenMs;
};

DedupEntry g_keys[kMaxKeys];
int        g_keyCount = 0;

// Per-tag state of BlockLog: the hash of the last block, not its text, and
// its first line to name it in the "(repeated Nx)" summary.
constexpr int kMaxBlockKeys = 16;

struct BlockEntry {
    char     tag[64];
    uint64_t lastHash;
    bool     hasLast;
    int      repeated;        // identical blocks since lastHash was written
    char     descriptor[96];
    DWORD    lastSeenMs;
};

BlockEntry g_blocks[kMaxBlockKeys];
int        g_blockCount = 0;

// FNV-1a, 64 bit.
constexpr uint64_t kFnvOffset = 1469598103934665603ull;
constexpr uint64_t kFnvPrime  = 1099511628211ull;

uint64_t FnvAdd(uint64_t h, char c) {
    return (h ^ static_cast<unsigned char>(c)) * kFnvPrime;
}

uint64_t Fnv1a64(const char* p, size_t n) {
    uint64_t h = kFnvOffset;
    for (size_t i = 0; i < n; ++i) h = FnvAdd(h, p[i]);
    return h;
}

// The slots are never reused and start zeroed, so a new one needs only its
// tag. Null when the table is full.
template <typename Entry, size_t N>
Entry* GetOrCreate(Entry (&table)[N], int& count, const char* tag) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(table[i].tag, tag) == 0) return &table[i];
    }
    if (count >= static_cast<int>(N)) return nullptr;
    Entry* e = &table[count++];
    strncpy_s(e->tag, tag, _TRUNCATE);
    return e;
}

// One line to the file, no dedup. Under the lock.
void RawWriteLocked(const char* tag, const char* content) {
    SYSTEMTIME ts;
    GetSystemTime(&ts);

    char line[kMessageBuf + 128];
    snprintf(line, sizeof(line),
        "%04d-%02d-%02dT%02d:%02d:%02dZ [accessibility] %s%s%s\n",
        ts.wYear, ts.wMonth, ts.wDay,
        ts.wHour, ts.wMinute, ts.wSecond,
        tag, *tag ? ": " : "", content);

    // The debug stream only with a debugger attached. OutputDebugStringA
    // takes a system-wide mutex (DBWinMutex) and, when a listener exists,
    // blocks on an ack event. Any background process that drains the debug
    // buffer (AV/EDR agents, DebugView, telemetry) can therefore stall every
    // call here, and since this runs for every log line on the game's main
    // tick, a contended drain freezes the game for seconds (audio underruns:
    // KOTOR refills its streaming buffers on that tick). IsDebuggerPresent is
    // one byte read, and nothing is lost: the file below has every line.
    if (IsDebuggerPresent()) OutputDebugStringA(line);

    if (!g_logOpenAttempted) {
        g_logOpenAttempted = true;
        CreateDirectoryA(g_logDir, nullptr);
        // Shared for reading (deny only other writers) so external tools can
        // follow the log while the game runs. fopen_s opens unshared.
        g_logFile = _fsopen(g_logPath, "ab", _SH_DENYWR);
        if (!g_logFile) {
            OutputDebugStringA("[accessibility] WARNING: log file open failed\n");
        }
    }
    if (g_logFile) {
        fputs(line, g_logFile);
        fflush(g_logFile);
    }
}

// The three flushes write a pending count, if any, and reset it. Under the
// lock.
void FlushTraceLocked(DedupEntry* e) {
    if (e->suppressed <= 0) return;
    char buf[128];
    snprintf(buf, sizeof(buf), "(repeated %dx more)", e->suppressed);
    RawWriteLocked(e->tag, buf);
    e->suppressed = 0;
}

void FlushEdgeHoldLocked(DedupEntry* e) {
    if (e->stateHeld <= 0) return;
    char buf[128];
    snprintf(buf, sizeof(buf), "(prev state held %dx)", e->stateHeld);
    RawWriteLocked(e->tag, buf);
    e->stateHeld = 0;
}

void FlushBlockLocked(BlockEntry* e) {
    if (e->repeated <= 0) return;
    char buf[160];
    snprintf(buf, sizeof(buf), "(repeated %dx — unchanged: %s)",
             e->repeated, e->descriptor);
    RawWriteLocked(e->tag, buf);
    e->repeated = 0;
}

// A '\n'-separated block as one tagged line per segment. Under the lock.
void EmitBlockLinesLocked(const char* tag, const char* buf) {
    const char* start = buf;
    while (*start) {
        const char* nl = strchr(start, '\n');
        if (!nl) { RawWriteLocked(tag, start); break; }
        char line[kMessageBuf];
        size_t n = static_cast<size_t>(nl - start);
        if (n >= sizeof(line)) n = sizeof(line) - 1;
        memcpy(line, start, n);
        line[n] = '\0';
        RawWriteLocked(tag, line);
        start = nl + 1;
    }
}

// Writes out the counts of tags that have gone quiet. Every public call runs
// it first, so the count of a subsystem that stopped firing still surfaces.
void SweepStaleLocked() {
    const DWORD now = GetTickCount();
    for (int i = 0; i < g_keyCount; ++i) {
        DedupEntry* e = &g_keys[i];
        if (e->lastSeenMs == 0 || now - e->lastSeenMs < kStaleMs) continue;
        FlushTraceLocked(e);
        FlushEdgeHoldLocked(e);
    }
    for (int i = 0; i < g_blockCount; ++i) {
        BlockEntry* e = &g_blocks[i];
        if (e->lastSeenMs == 0 || now - e->lastSeenMs < kStaleMs) continue;
        FlushBlockLocked(e);
    }
}

// The entry for tag. With the table full the message is written plainly and
// null returned, so nothing is lost. Under the lock.
DedupEntry* EntryOrWriteLocked(const char* tag, const char* message) {
    DedupEntry* e = GetOrCreate(g_keys, g_keyCount, tag);
    if (!e) RawWriteLocked(tag, message);
    return e;
}

}  // namespace

void Init(HINSTANCE hinstDLL) {
    if (!g_lockInited) {
        InitializeCriticalSection(&g_lock);
        g_lockInited = true;
    }

    char dllPath[MAX_PATH] = "";
    DWORD n = GetModuleFileNameA(hinstDLL, dllPath, sizeof(dllPath));
    if (n == 0 || n >= sizeof(dllPath)) {
        OutputDebugStringA(
            "[accessibility] GetModuleFileNameA failed; using CWD-relative paths\n");
        return;
    }

    char* slash = strrchr(dllPath, '\\');
    if (!slash) return;
    *slash = '\0';                          // <install>\patches
    strncpy_s(g_patchDir, dllPath, _TRUNCATE);

    slash = strrchr(dllPath, '\\');
    if (!slash) return;
    *slash = '\0';                          // <install>

    snprintf(g_logDir, sizeof(g_logDir), "%s\\logs", dllPath);

    SYSTEMTIME utc, local;
    GetSystemTime(&utc);
    GetLocalTime(&local);
    snprintf(g_logPath, sizeof(g_logPath),
        "%s\\patch-%04d%02d%02d-%02d%02d%02d.log",
        g_logDir,
        utc.wYear, utc.wMonth, utc.wDay,
        utc.wHour, utc.wMinute, utc.wSecond);

    char dbg[MAX_PATH + 64];
    snprintf(dbg, sizeof(dbg), "[accessibility] log path: %s\n", g_logPath);
    OutputDebugStringA(dbg);

    // A header that makes the log readable on its own. Speech and the version
    // SHA report themselves later; this is what is known now.
    char locale[16] = "?";
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SNAME, locale, sizeof(locale));
    Locker lk;
    char hdr[MAX_PATH + 16];
    snprintf(hdr, sizeof(hdr),
        "session start utc=%04d-%02d-%02dT%02d:%02d:%02dZ "
        "local=%02d:%02d:%02d locale=%s pid=%lu",
        utc.wYear, utc.wMonth, utc.wDay,
        utc.wHour, utc.wMinute, utc.wSecond,
        local.wHour, local.wMinute, local.wSecond,
        locale, GetCurrentProcessId());
    RawWriteLocked("Init.header", hdr);
    snprintf(hdr, sizeof(hdr), "log path=%s", g_logPath);
    RawWriteLocked("Init.header", hdr);
}

void Shutdown() {
    if (!g_lockInited) return;
    Locker l;
    SweepStaleLocked();
    for (int i = 0; i < g_keyCount; ++i) {
        FlushTraceLocked(&g_keys[i]);
        FlushEdgeHoldLocked(&g_keys[i]);
    }
    for (int i = 0; i < g_blockCount; ++i) FlushBlockLocked(&g_blocks[i]);
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = nullptr;
    }
    // The lock stays: a late write from another thread during
    // DLL_PROCESS_DETACH must still find it.
}

void Write(const char* tag, const char* fmt, ...) {
    if (!fmt) return;

    char message[kMessageBuf];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    Locker l;
    SweepStaleLocked();
    RawWriteLocked(tag ? tag : "", message);
}

void Trace(const char* tag, const char* fmt, ...) {
    if (!tag || !*tag || !fmt) return;

    char message[kMessageBuf];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    Locker l;
    SweepStaleLocked();
    DedupEntry* e = EntryOrWriteLocked(tag, message);
    if (!e) return;

    e->lastSeenMs = GetTickCount();
    if (e->hasContent && strcmp(e->lastContent, message) == 0) {
        e->suppressed++;
        return;
    }
    FlushTraceLocked(e);
    RawWriteLocked(tag, message);
    strncpy_s(e->lastContent, message, _TRUNCATE);
    e->hasContent = true;
}

void Once(const char* tag, const char* fmt, ...) {
    if (!tag || !*tag || !fmt) return;

    char message[kMessageBuf];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    Locker l;
    SweepStaleLocked();
    DedupEntry* e = EntryOrWriteLocked(tag, message);
    if (!e || e->isOnce) return;
    RawWriteLocked(tag, message);
    e->isOnce     = true;
    e->lastSeenMs = GetTickCount();
}

void Edge(const char* tag, int state, const char* fmt, ...) {
    if (!tag || !*tag || !fmt) return;

    char message[kMessageBuf];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    Locker l;
    SweepStaleLocked();
    DedupEntry* e = EntryOrWriteLocked(tag, message);
    if (!e) return;

    e->lastSeenMs = GetTickCount();
    if (e->hasState && e->state == state) {
        e->stateHeld++;
        return;
    }
    FlushEdgeHoldLocked(e);
    RawWriteLocked(tag, message);
    e->state    = state;
    e->hasState = true;
}

BlockLog::BlockLog(const char* tag)
    : tag_(tag ? tag : ""), len_(0), ended_(false), passthrough_(false),
      hasKey_(false), keyHash_(kFnvOffset) {
    buf_[0] = '\0';
}

BlockLog::~BlockLog() { End(); }

void BlockLog::Key(const char* fmt, ...) {
    if (ended_ || !fmt) return;
    char msg[kMessageBuf];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    // Into the running hash, with a '\n' after each key so two keys cannot
    // run together.
    for (const char* p = msg; *p; ++p) keyHash_ = FnvAdd(keyHash_, *p);
    keyHash_ = FnvAdd(keyHash_, '\n');
    hasKey_ = true;
}

void BlockLog::Line(const char* fmt, ...) {
    if (ended_ || !fmt) return;

    char msg[kMessageBuf];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    if (passthrough_) {
        Write(tag_, "%s", msg);
        return;
    }

    size_t mlen = strlen(msg);
    if (len_ + mlen + 2 >= kCap) {
        // The block outgrew the buffer: write what there is and this line,
        // and the rest goes straight to the file, unfolded.
        Locker l;
        EmitBlockLinesLocked(tag_, buf_);
        RawWriteLocked(tag_, msg);
        passthrough_ = true;
        return;
    }
    memcpy(buf_ + len_, msg, mlen);
    len_ += mlen;
    buf_[len_++] = '\n';
    buf_[len_]   = '\0';
}

void BlockLog::End() {
    if (ended_) return;
    ended_ = true;
    if (passthrough_ || len_ == 0) return;   // already written, or nothing to write

    // Identity is the Key() stream when one was given (it leaves out noise
    // such as heap pointers), otherwise the display text.
    const uint64_t h = hasKey_ ? keyHash_ : Fnv1a64(buf_, len_);

    Locker l;
    SweepStaleLocked();

    BlockEntry* e = GetOrCreate(g_blocks, g_blockCount, tag_);
    if (!e) {                                 // table full: write, do not fold
        EmitBlockLinesLocked(tag_, buf_);
        return;
    }

    e->lastSeenMs = GetTickCount();
    if (e->hasLast && e->lastHash == h) {
        e->repeated++;
        return;
    }

    FlushBlockLocked(e);
    EmitBlockLinesLocked(tag_, buf_);

    // The first line names the block in the next fold's summary.
    const char* nl = strchr(buf_, '\n');
    size_t dn = nl ? static_cast<size_t>(nl - buf_) : strlen(buf_);
    if (dn >= sizeof(e->descriptor)) dn = sizeof(e->descriptor) - 1;
    memcpy(e->descriptor, buf_, dn);
    e->descriptor[dn] = '\0';

    e->lastHash = h;
    e->hasLast  = true;
}

const char* PatchDir() {
    return g_patchDir;
}

void BringupMark(const char* name) {
    ULONGLONG now = GetTickCount64();
    if (g_bringupBaselineMs == 0) {
        g_bringupBaselineMs = now;
        Write("Bringup", "%s t+0ms (baseline)", name);
        return;
    }
    Write("Bringup", "%s t+%llums", name,
          static_cast<unsigned long long>(now - g_bringupBaselineMs));
}

}  // namespace acclog
