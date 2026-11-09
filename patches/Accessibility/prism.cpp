// Speech bridge — Prism only. Loads prism.dll, picks the screen-reader
// backend (whichever Prism picks at startup; acquire_best) and sends it every
// line.
//
// Braille: Tolk's Tolk_Output spoke and brailled in one call; Prism splits
// those. Speech therefore goes through DispatchNormal — prism_backend_output
// (the Tolk_Output equivalent) where the backend offers it, else speak plus a
// braille flash message.
//
// prism.dll is loaded at runtime, so a missing one leaves the mod silent
// instead of failing to load.

#include "prism_internal.h"

#include "log.h"

namespace prism {

namespace {

// PrismBackendFeature bits. IS_SUPPORTED_AT_RUNTIME is set only while the
// backend's reader is genuinely reachable — every Windows backend recomputes it
// on each call (NVDA queries its RPC endpoint, SAPI asks COM for the SpVoice
// class object, ZDSR and BoyPC scan the process list). Unlike a successful
// initialize() it is trustworthy; BackendIsUsable explains why that matters.
constexpr uint64_t kPrismFeatureSupportedAtRuntime = 1ull << 0;
constexpr uint64_t kPrismFeatureSupportsSpeak      = 1ull << 2;
// Braille-only entry point (prism_backend_braille).
constexpr uint64_t kPrismFeatureSupportsBraille    = 1ull << 4;
// Combined speech+braille entry point (prism_backend_output) — the
// Tolk_Output equivalent.
constexpr uint64_t kPrismFeatureSupportsOutput     = 1ull << 5;

// MSVC delay-load helper exceptions — see the SEH guards below.
constexpr DWORD kVcppDelayLoadModNotFound  = 0xC06D007E;
constexpr DWORD kVcppDelayLoadProcNotFound = 0xC06D007F;

HMODULE g_prismLib  = nullptr;
bool    g_initTried = false;

// Braille channels the normal backend advertised when it was adopted. Either
// flag can latch off later if a call comes back NOT_IMPLEMENTED — an older
// vendor client DLL can advertise a bit its export set cannot honour.
bool g_normalSupportsOutput  = false;  // combined speak+braille call (Tolk_Output equivalent)
bool g_normalSupportsBraille = false;  // braille-only call

template <typename T>
bool Resolve(HMODULE lib, T& fn, const char* name) {
    fn = reinterpret_cast<T>(GetProcAddress(lib, name));
    if (!fn) acclog::Write("Speech", "GetProcAddress(%s) failed", name);
    return fn != nullptr;
}

// ---- SEH guards around prism backend calls -----------------------------
//
// Some screen-reader backends delay-load a vendor DLL inside their
// initialize() path (ZDSR → ZDSRAPI.dll, PC-Talker → PCTKUSR.dll, BoyPC →
// BoyCtrl.dll, ...). When the user has that reader installed at a DLL version
// whose export set doesn't match what prism.dll was built against, the MSVC
// delay-load helper raises a structured exception (PROC_NOT_FOUND /
// MOD_NOT_FOUND) that would crash the game at startup. Prism's own
// acquire_best guards each initialize() and skips a faulting backend; these
// wrappers guard the registry and initialize calls this module makes itself.
// They hold only PODs and raw pointers — MSVC forbids __try in a function that
// needs C++ object unwinding — so the callers log the returned fault code.

const char* SehCodeName(DWORD code) {
    switch (code) {
        case kVcppDelayLoadModNotFound:  return "delay-load module-not-found";
        case kVcppDelayLoadProcNotFound: return "delay-load proc-not-found";
        case EXCEPTION_ACCESS_VIOLATION: return "access-violation";
        default:                         return "structured-exception";
    }
}

// Returns acquire_best's backend, or null. *outCode is the SEH code if a fault
// was caught (0 on a clean null/success).
PrismBackend* SehAcquireBest(PrismContext* ctx, DWORD* outCode) {
    *outCode = 0;
    __try {
        return pPrism_registry_acquire_best(ctx);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *outCode = GetExceptionCode();
        return nullptr;
    }
}

// Acquire one specific backend by id, SEH-guarded.
PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode) {
    *outCode = 0;
    __try {
        return pPrism_registry_acquire(ctx, id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *outCode = GetExceptionCode();
        return nullptr;
    }
}

// Initialize one backend, SEH-guarded. Returns the PrismError, or
// kPrismErrUnknown with *outCode set if a fault was caught.
PrismError SehInitialize(PrismBackend* be, DWORD* outCode) {
    *outCode = 0;
    __try {
        return pPrism_backend_initialize(be);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *outCode = GetExceptionCode();
        return kPrismErrUnknown;
    }
}

// True iff this backend can speak AND its reader is reachable right now.
//
// A successful initialize() is not enough: Prism 0.16.5's NVDA backend
// returned success when NVDA was not running, and as the highest-priority
// backend it then won acquire_best and refused every utterance — total
// silence for anyone running JAWS, Narrator, ZDSR or nothing at all. Fixed
// upstream in 0.17.x; the gate costs one call and is correct either way.
bool BackendIsUsable(PrismBackend* be) {
    if (!be) return false;
    const uint64_t want = kPrismFeatureSupportsSpeak | kPrismFeatureSupportedAtRuntime;
    return (pPrism_backend_get_features(be) & want) == want;
}

// Install `be` as the normal backend and record which braille channels it
// offers, so Speak can reach a braille display and not just the synthesizer.
void AdoptNormal(PrismBackend* be) {
    g_prismNormal = be;
    const uint64_t features = pPrism_backend_get_features(be);
    g_normalSupportsOutput  = (features & kPrismFeatureSupportsOutput)  != 0;
    g_normalSupportsBraille = (features & kPrismFeatureSupportsBraille) != 0;
}

// How the adopted backend reaches a braille display, for the log — the line a
// braille user's report gets checked against. Keyed on SUPPORTS_BRAILLE, not
// SUPPORTS_OUTPUT: every backend advertises output() (on the speech-only ones
// its braille half is a no-op), so only the braille bit says whether a display
// can actually be reached.
const char* BrailleModeText() {
    if (!g_normalSupportsBraille) return "no braille";
    return g_normalSupportsOutput ? "braille via output()" : "braille supported";
}

// Mirrors text to the braille display, when the backend has one. Quiet by
// design: braille is supplementary on this path, so a failure must never cost
// speech or spam the log — only a NOT_IMPLEMENTED is noted, once, when
// latching the capability off. `utf8` must already be UTF-8.
void BrailleNormal(const char* utf8) {
    if (!g_normalSupportsBraille) return;
    PrismError rc = pPrism_backend_braille(g_prismNormal, utf8);
    if (rc == kPrismErrNotImplemented) {
        // Advertised but not really there — e.g. a ZDSRAPI predating its
        // Braille export.
        g_normalSupportsBraille = false;
        acclog::Write("Speech",
                      "normal backend advertises braille but reports "
                      "NotImplemented; braille off for this backend");
    }
}

// Sends one announcement to the normal backend on every channel it offers: a
// backend advertising SUPPORTS_OUTPUT gets prism_backend_output, the rest get
// speak plus, where supported, a braille flash message.
PrismError DispatchNormal(const char* utf8, bool interrupt) {
    if (g_normalSupportsOutput) {
        PrismError rc = pPrism_backend_output(g_prismNormal, utf8, interrupt);
        if (rc != kPrismErrNotImplemented) return rc;

        // The feature bit promised output() but the call disagreed — an older
        // vendor client DLL can do that. Take speak (+ braille) from here on.
        g_normalSupportsOutput = false;
        acclog::Write("Speech",
                      "normal backend advertises output() but reports "
                      "NotImplemented; switching to speak (%s)",
                      BrailleModeText());
    }
    PrismError rcSpeak = pPrism_backend_speak(g_prismNormal, utf8, interrupt);
    BrailleNormal(utf8);
    return rcSpeak;
}

void SpeakNormalUtf8(const char* utf8, bool interrupt) {
    const PrismError rc = DispatchNormal(utf8, interrupt);
    if (rc != kPrismOk) acclog::Trace("Speech", "normal dispatch rc=%d; dropping utterance", rc);
}

// Walk the registry ourselves and take the first backend that is live and
// initialises. Registry order is descending priority, so this reproduces
// acquire_best's ordering — which cannot simply be called again, because it
// caches its first pick and would hand back the same dead backend.
//
// Liveness is checked before initialize(), never after, so a reader that is
// not running never has its vendor client library touched. That matters here:
// this walk is the path that reaches ZDSR / PC-Talker / BoyPC, and skipping
// them outright is stronger than relying on the SEH guard to survive them.
PrismBackend* WalkForLiveBackend() {
    const size_t count = pPrism_registry_count(g_prismCtx);
    for (size_t i = 0; i < count; ++i) {
        const PrismBackendId id = pPrism_registry_id_at(g_prismCtx, i);
        if (id == 0) continue;

        DWORD seh = 0;
        PrismBackend* be = SehAcquire(g_prismCtx, id, &seh);
        if (!BackendIsUsable(be)) continue;

        const PrismError rc = SehInitialize(be, &seh);
        if (seh) {
            acclog::Write("Speech",
                          "prism_backend_initialize faulted for a live backend "
                          "(%s 0x%08lX); trying the next one",
                          SehCodeName(seh), seh);
            continue;
        }
        if (!InitOk(rc)) {
            acclog::Write("Speech",
                          "backend reported itself live but initialize failed "
                          "rc=%d; trying the next one", rc);
            continue;
        }
        return be;
    }
    return nullptr;
}

// Pick the screen-reader / TTS backend. Prism's acquire_best gets first say —
// it owns the priority walk and skips a backend whose vendor DLL faults during
// initialize() — but whatever it returns still has to pass BackendIsUsable,
// and WalkForLiveBackend takes over when it does not. SAPI remains the
// catch-all at the bottom of the order, so a machine with no screen reader
// running still speaks.
bool TryAcquireNormal() {
    DWORD seh = 0;
    PrismBackend* best = SehAcquireBest(g_prismCtx, &seh);
    if (seh) {
        acclog::Write("Speech",
                      "prism_registry_acquire_best faulted (%s 0x%08lX); "
                      "walking the registry instead", SehCodeName(seh), seh);
    }

    if (BackendIsUsable(best)) {
        AdoptNormal(best);
        return true;
    }

    if (best) {
        const char* nameA = pPrism_backend_name(best);
        acclog::Write("Speech",
                      "acquire_best chose %s but its reader is not running; "
                      "walking the registry", nameA ? nameA : "?");
    }

    PrismBackend* walked = WalkForLiveBackend();
    if (walked) {
        AdoptNormal(walked);
        return true;
    }

    acclog::Write("Speech", "no live speech backend available; running silent");
    return false;
}

}  // namespace

bool Init() {
    if (g_initTried) return g_normalReady;
    g_initTried = true;

    const char* patchDir = acclog::PatchDir();
    if (!patchDir || !*patchDir) {
        acclog::Write("Speech", "patch dir unknown; aborting init");
        return false;
    }

    char path[MAX_PATH];
    int written = snprintf(path, sizeof(path), "%s\\prism.dll", patchDir);
    if (written <= 0 || written >= (int)sizeof(path)) {
        acclog::Write("Speech", "prism.dll path overflow");
        return false;
    }

    // Point the DLL search path at our dir so prism.dll's delay-loaded
    // bridges resolve against our bundled neighbours instead of whatever is
    // in WindowsApps. Restored after the LoadLibrary call.
    char prevDir[MAX_PATH] = {0};
    DWORD prevLen = GetDllDirectoryA(MAX_PATH, prevDir);
    SetDllDirectoryA(patchDir);
    g_prismLib = LoadLibraryA(path);
    const DWORD loadErr = GetLastError();
    SetDllDirectoryA(prevLen > 0 ? prevDir : nullptr);
    if (!g_prismLib) {
        acclog::Write("Speech",
                      "LoadLibrary(%s) failed err=%lu; running silent", path, loadErr);
        return false;
    }

    const bool ok =
        Resolve(g_prismLib, pPrism_init,                  "prism_init")                  &&
        Resolve(g_prismLib, pPrism_registry_count,        "prism_registry_count")        &&
        Resolve(g_prismLib, pPrism_registry_id_at,        "prism_registry_id_at")        &&
        Resolve(g_prismLib, pPrism_registry_acquire,      "prism_registry_acquire")      &&
        Resolve(g_prismLib, pPrism_registry_acquire_best, "prism_registry_acquire_best") &&
        Resolve(g_prismLib, pPrism_backend_get_features,  "prism_backend_get_features")  &&
        Resolve(g_prismLib, pPrism_backend_initialize,    "prism_backend_initialize")    &&
        Resolve(g_prismLib, pPrism_backend_speak,         "prism_backend_speak")         &&
        Resolve(g_prismLib, pPrism_backend_output,        "prism_backend_output")        &&
        Resolve(g_prismLib, pPrism_backend_braille,       "prism_backend_braille")       &&
        Resolve(g_prismLib, pPrism_backend_stop,          "prism_backend_stop")          &&
        Resolve(g_prismLib, pPrism_backend_name,          "prism_backend_name");
    if (!ok) return false;

    // NULL config on purpose — see the PFN_prism_init declaration.
    g_prismCtx = pPrism_init(nullptr);
    if (!g_prismCtx) {
        acclog::Write("Speech", "prism_init returned NULL; running silent");
        return false;
    }

    g_normalReady = TryAcquireNormal();
    if (g_normalReady) {
        const char* nameA = pPrism_backend_name(g_prismNormal);
        acclog::Write("Speech", "ready — backend = %s (%s)", nameA ? nameA : "?",
                      BrailleModeText());
    }
    return g_normalReady;
}

bool IsAvailable() {
    return g_normalReady;
}

// Every Speak call is logged so the patch log carries one canonical record of
// what the screen reader was asked to say, in order. "[!]" marks
// interrupt=true. Trace dedups consecutive identical strings.
void Speak(const wchar_t* text, bool interrupt) {
    if (!g_normalReady || !text || !*text) return;
    const std::string utf8 = WideToUtf8(text);
    if (!utf8.empty()) acclog::Trace("Speech.spoke", "%s%s", interrupt ? "[!] " : "", utf8.c_str());
    CaptureSpokenW(text);
    if (!utf8.empty()) SpeakNormalUtf8(utf8.c_str(), interrupt);
}

void Speak(const char* text, bool interrupt) {
    if (!g_normalReady || !text || !*text) return;
    acclog::Trace("Speech.spoke", "%s%s", interrupt ? "[!] " : "", text);
    CaptureSpoken(text);
    const std::string utf8 = ToUtf8(text);
    if (!utf8.empty()) SpeakNormalUtf8(utf8.c_str(), interrupt);
}

void Silence() {
    if (!g_normalReady) return;
    acclog::Trace("Speech.spoke", "(silenced)");
    (void)pPrism_backend_stop(g_prismNormal);
}

void Shutdown() {
    // Deliberately do not free backends / shut down the context here —
    // Prism's bridge DLLs talk to RPC + COM objects, and tearing those down
    // inside DLL_PROCESS_DETACH risks loader-lock + COM-uninit ordering
    // issues. Process exit will reclaim everything.
    g_normalReady = false;
}

}  // namespace prism
