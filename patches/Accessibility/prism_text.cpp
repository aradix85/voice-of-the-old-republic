// Text for the speech channels: re-encoding between the game's codepage and
// Prism's UTF-8, and the last line spoken (for the bridge and repeat-last).

#include "prism_internal.h"

#include "bridge.h"   // spoken-line ring buffer for the local bridge
#include "log.h"

namespace prism {

namespace {

// Active narrow-text codepage. Both our own string tables and the engine's
// CExoString payload are in the *game's* codepage, not necessarily the OS one:
// a Russian install is Windows-1251 even on a German or English Windows, where
// CP_ACP is 1252. Defaulting to CP_ACP keeps en/de/fr/it/es byte-for-byte
// (those are all 1252, which is what CP_ACP resolves to on their users'
// systems); SetSpeechCodepage pins it explicitly once the language is known.
// See DetectLanguageFromTlk in core_dllmain.cpp.
UINT g_speechCodepage = CP_ACP;

// Last-utterance capture, held in the game codepage — the same bytes the
// caller handed us — so consumers can re-widen with the codepage of their
// choice. Sized well above kAnnounceTextMax (1024) so we never truncate an
// utterance the speech path itself carried in full; over-long text is
// truncated rather than dropped.
char s_lastSpoken[4096] = {0};
bool s_suppressNextCapture = false;

// `text` in codepage `cp`; empty when it cannot be converted. Best-fit
// characters are allowed: an approximation beats losing the line.
std::string Narrow(const wchar_t* text, UINT cp) {
    const int n = WideCharToMultiByte(cp, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    if (WideCharToMultiByte(cp, 0, text, -1, &out[0], n, nullptr, nullptr) <= 0) return {};
    out.resize(static_cast<size_t>(n) - 1);
    return out;
}

// `in` from codepage `from` to codepage `to`, through UTF-16.
std::string Recode(const char* in, UINT from, UINT to) {
    const int n = MultiByteToWideChar(from, 0, in, -1, nullptr, 0);
    if (n <= 0) return {};
    std::wstring wide(static_cast<size_t>(n), L'\0');
    if (MultiByteToWideChar(from, 0, in, -1, &wide[0], n) <= 0) return {};
    return Narrow(wide.c_str(), to);
}

}  // namespace

// Game codepage → UTF-8. Prism's SAPI backend strict-validates UTF-8 and
// returns PRISM_ERROR_INVALID_UTF8 on any non-UTF-8 lead byte, and the NVDA
// backend runs text through UTF-8 validators too, so without this every line
// with a non-ASCII byte (umlauts, Cyrillic, ...) would be dropped.
std::string ToUtf8(const char* gameText) {
    return Recode(gameText, g_speechCodepage, CP_UTF8);
}

// Prism hands voice names back in UTF-8; everything downstream of us (speech
// dispatch, the composed UI line) works in the game codepage.
std::string FromUtf8(const char* utf8) {
    return Recode(utf8, CP_UTF8, g_speechCodepage);
}

std::string WideToUtf8(const wchar_t* text) {
    return Narrow(text, CP_UTF8);
}

void CaptureSpoken(const char* text) {
    if (s_suppressNextCapture) {
        s_suppressNextCapture = false;
        return;
    }
    strncpy_s(s_lastSpoken, text, _TRUNCATE);
    ::acc::bridge::RecordSpoken(s_lastSpoken);
}

// Folded back to the game codepage, so LastSpoken() has one encoding whichever
// overload produced it.
void CaptureSpokenW(const wchar_t* text) {
    CaptureSpoken(Narrow(text, g_speechCodepage).c_str());
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
