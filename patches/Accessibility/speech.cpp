// Speech and braille through the NVDA controller client: every line goes to
// NVDA as speech and as a braille flash message. The client talks to NVDA over
// local RPC; while NVDA is not running its calls fail and the line is dropped.

#include "speech.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>

#include "bridge.h"   // spoken-line ring buffer for the local bridge
#include "log.h"

namespace acc::speech {

namespace {

// nvdaController.h (third_party/nvda/include): every call returns 0 on success.
typedef unsigned long (__stdcall* PFN_TestIfRunning)();
typedef unsigned long (__stdcall* PFN_SpeakText)(const wchar_t* text);
typedef unsigned long (__stdcall* PFN_CancelSpeech)();
typedef unsigned long (__stdcall* PFN_BrailleMessage)(const wchar_t* message);

PFN_SpeakText      pSpeakText      = nullptr;
PFN_CancelSpeech   pCancelSpeech   = nullptr;
PFN_BrailleMessage pBrailleMessage = nullptr;
bool g_initTried = false;
bool g_ready     = false;

// Last-utterance capture, in the game codepage. Sized well above
// kAnnounceTextMax (1024) so an utterance is never truncated; over-long text
// is truncated rather than dropped.
char s_lastSpoken[4096] = {0};
bool s_suppressNextCapture = false;

template <typename T>
bool Resolve(HMODULE lib, T& fn, const char* name) {
    fn = reinterpret_cast<T>(GetProcAddress(lib, name));
    if (!fn) acclog::Write("Speech", "GetProcAddress(%s) failed", name);
    return fn != nullptr;
}

std::wstring Widen(const char* text) {
    const int n = MultiByteToWideChar(kGameCodepage, 0, text, -1, nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    if (MultiByteToWideChar(kGameCodepage, 0, text, -1, &out[0], n) <= 0) return {};
    out.resize(static_cast<size_t>(n) - 1);
    return out;
}

// To the game codepage; best-fit characters are allowed, since an
// approximation beats losing the line.
std::string Narrow(const wchar_t* text) {
    const int n = WideCharToMultiByte(kGameCodepage, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    if (WideCharToMultiByte(kGameCodepage, 0, text, -1, &out[0], n, nullptr, nullptr) <= 0)
        return {};
    out.resize(static_cast<size_t>(n) - 1);
    return out;
}

void Capture(const char* text) {
    if (s_suppressNextCapture) {
        s_suppressNextCapture = false;
        return;
    }
    strncpy_s(s_lastSpoken, text, _TRUNCATE);
    ::acc::bridge::RecordSpoken(s_lastSpoken);
}

// The patch log carries one canonical record of what NVDA was asked to say,
// in order; "[!]" marks interrupt=true. Trace dedups consecutive identical
// lines.
void Send(const char* logText, const wchar_t* text, bool interrupt) {
    acclog::Trace("Speech.spoke", "%s%s", interrupt ? "[!] " : "", logText);
    if (interrupt) pCancelSpeech();
    const unsigned long rc = pSpeakText(text);
    pBrailleMessage(text);
    if (rc != 0) acclog::Trace("Speech", "NVDA did not take the line (rc=%lu)", rc);
}

}  // namespace

bool Init() {
    if (g_initTried) return g_ready;
    g_initTried = true;

    const char* patchDir = acclog::PatchDir();
    if (!patchDir || !*patchDir) {
        acclog::Write("Speech", "patch dir unknown; running silent");
        return false;
    }
    char path[MAX_PATH];
    const int written = snprintf(path, sizeof(path), "%s\\nvdaControllerClient32.dll", patchDir);
    if (written <= 0 || written >= static_cast<int>(sizeof(path))) {
        acclog::Write("Speech", "controller client path overflow; running silent");
        return false;
    }
    HMODULE lib = LoadLibraryA(path);
    if (!lib) {
        acclog::Write("Speech", "LoadLibrary(%s) failed err=%lu; running silent", path,
                      GetLastError());
        return false;
    }
    PFN_TestIfRunning testIfRunning = nullptr;
    g_ready = Resolve(lib, testIfRunning,   "nvdaController_testIfRunning") &&
              Resolve(lib, pSpeakText,      "nvdaController_speakText")     &&
              Resolve(lib, pCancelSpeech,   "nvdaController_cancelSpeech")  &&
              Resolve(lib, pBrailleMessage, "nvdaController_brailleMessage");
    if (g_ready) {
        acclog::Write("Speech", "NVDA controller client ready; NVDA %s",
                      testIfRunning() == 0 ? "is running" : "is not running yet");
    }
    return g_ready;
}

void Speak(const wchar_t* text, bool interrupt) {
    if (!g_ready || !text || !*text) return;
    const std::string narrow = Narrow(text);
    Capture(narrow.c_str());
    Send(narrow.c_str(), text, interrupt);
}

void Speak(const char* text, bool interrupt) {
    if (!g_ready || !text || !*text) return;
    Capture(text);
    const std::wstring wide = Widen(text);
    if (!wide.empty()) Send(text, wide.c_str(), interrupt);
}

void Silence() {
    if (!g_ready) return;
    acclog::Trace("Speech.spoke", "(silenced)");
    pCancelSpeech();
}

const char* LastSpoken() { return s_lastSpoken; }

void SuppressNextCapture() { s_suppressNextCapture = true; }

}  // namespace acc::speech
