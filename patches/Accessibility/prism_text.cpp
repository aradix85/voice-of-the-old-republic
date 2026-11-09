// Text for the speech channels: re-encoding between the game's codepage and
// Prism's UTF-8, and the last line spoken (for the bridge and repeat-last).

#include "prism_internal.h"

namespace prism {

namespace {

// Active narrow-text codepage. Both our own string tables and the engine's
// CExoString payload are in the *game's* codepage, not necessarily the OS one:
// a Russian install is Windows-1251 even on a German or English Windows, where
// CP_ACP is 1252. Defaulting to CP_ACP keeps the pre-existing en/de/fr/it/es
// behaviour byte-for-byte (those are all 1252, which is what CP_ACP resolves to
// on their users' systems); SetSpeechCodepage pins it explicitly once the
// language is known. See DetectLanguageFromTlk in core_dllmain.cpp.
UINT g_speechCodepage = CP_ACP;

}  // namespace

// Game codepage → UTF-8. Prism's SAPI backend strict-validates UTF-8 and
// returns PRISM_ERROR_INVALID_UTF8 on any non-UTF-8 lead byte. The NVDA backend
// also strings text through simdutf-style validators in places. So any
// non-ASCII byte (German umlauts, Cyrillic, ...) is an invalid UTF-8 lead →
// whole utterance dropped without this re-encode.
//
// Writes a NUL-terminated UTF-8 string into `outBuf`. Returns true on success.
bool ReencodeAcpToUtf8(const char* in, char* outBuf, size_t outBufSize) {
    if (!in || !outBuf || outBufSize == 0) return false;
    const UINT cp = g_speechCodepage;
    int wideLen = MultiByteToWideChar(cp, 0, in, -1, nullptr, 0);
    if (wideLen <= 0) return false;
    wchar_t wStack[256];
    wchar_t* w = wStack;
    wchar_t* wHeap = nullptr;
    if (static_cast<size_t>(wideLen) > sizeof(wStack) / sizeof(wStack[0])) {
        wHeap = (wchar_t*)malloc(static_cast<size_t>(wideLen) * sizeof(wchar_t));
        if (!wHeap) return false;
        w = wHeap;
    }
    int gotWide = MultiByteToWideChar(cp, 0, in, -1, w, wideLen);
    if (gotWide <= 0) {
        if (wHeap) free(wHeap);
        return false;
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 0 || static_cast<size_t>(needed) > outBufSize) {
        if (wHeap) free(wHeap);
        return false;
    }
    int got = WideCharToMultiByte(CP_UTF8, 0, w, -1, outBuf,
                                  static_cast<int>(outBufSize),
                                  nullptr, nullptr);
    if (wHeap) free(wHeap);
    return got > 0;
}

// Mirror of the above: Prism hands voice names back in UTF-8, and everything
// downstream of us (speech dispatch, the composed UI line) works in the game
// codepage. Fails rather than truncating, same contract as ReencodeAcpToUtf8.
bool ReencodeUtf8ToGameCodepage(const char* in, char* outBuf, size_t outBufSize) {
    if (!in || !outBuf || outBufSize == 0) return false;
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, in, -1, nullptr, 0);
    if (wideLen <= 0) return false;
    wchar_t wStack[256];
    wchar_t* w = wStack;
    wchar_t* wHeap = nullptr;
    if (static_cast<size_t>(wideLen) > sizeof(wStack) / sizeof(wStack[0])) {
        wHeap = (wchar_t*)malloc(static_cast<size_t>(wideLen) * sizeof(wchar_t));
        if (!wHeap) return false;
        w = wHeap;
    }
    int gotWide = MultiByteToWideChar(CP_UTF8, 0, in, -1, w, wideLen);
    if (gotWide <= 0) {
        if (wHeap) free(wHeap);
        return false;
    }
    int needed = WideCharToMultiByte(g_speechCodepage, 0, w, -1, nullptr, 0,
                                     nullptr, nullptr);
    if (needed <= 0 || static_cast<size_t>(needed) > outBufSize) {
        if (wHeap) free(wHeap);
        return false;
    }
    int got = WideCharToMultiByte(g_speechCodepage, 0, w, -1, outBuf,
                                  static_cast<int>(outBufSize), nullptr, nullptr);
    if (wHeap) free(wHeap);
    return got > 0;
}

void CaptureSpoken(const char* text) {
    if (s_suppressNextCapture) {
        s_suppressNextCapture = false;
        return;
    }
    if (!text) return;
    strncpy_s(s_lastSpoken, text, _TRUNCATE);
    ::acc::bridge::RecordSpoken(s_lastSpoken);
}

// Wide path: fold back to the game codepage so LastSpoken() has one encoding
// regardless of which overload produced it. WC_NO_BEST_FIT_CHARS is
// deliberately NOT set — a best-fit approximation is better than losing the
// utterance for a codepoint the game codepage can't represent.
void CaptureSpokenW(const wchar_t* text) {
    if (s_suppressNextCapture) {
        s_suppressNextCapture = false;
        return;
    }
    if (!text) return;
    int got = WideCharToMultiByte(g_speechCodepage, 0, text, -1, s_lastSpoken,
                                  (int)sizeof(s_lastSpoken), nullptr, nullptr);
    if (got <= 0) s_lastSpoken[0] = '\0';
    s_lastSpoken[sizeof(s_lastSpoken) - 1] = '\0';
    ::acc::bridge::RecordSpoken(s_lastSpoken);
}

void SetSpeechCodepage(unsigned codepage) {
    if (codepage == 0) return;
    if (g_speechCodepage == codepage) return;
    g_speechCodepage = static_cast<UINT>(codepage);
    acclog::Write("Speech", "narrow-text codepage set to %u", codepage);
}

unsigned GetSpeechCodepage() { return g_speechCodepage; }

const char* LastSpoken() { return s_lastSpoken; }

void SuppressNextCapture() { s_suppressNextCapture = true; }

}  // namespace prism
