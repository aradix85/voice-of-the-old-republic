// The compass channel: a second SAPI instance with its own voice, volume and
// rate for the camera-turn readout, and the follow-up line spoken on the normal
// channel once the compass voice has finished.

#include "prism_internal.h"

namespace prism {

namespace {

// A persisted percent [0,100], pulled from the store on first access.
struct PercentSetting {
    const char* key;
    int         value;   // the default until loaded
    bool        loaded;
};
PercentSetting g_compassVolume = {"CompassVolumePercent", 100, false};
PercentSetting g_compassRate   = {"CompassRatePercent", kSapiRateDefaultPercent, false};

// The normal-channel line SpeakCompassThen holds back until the compass voice
// has said its words, so the two voices take turns instead of talking at once.
// SAPI reports "speaking" only once audio starts, so a voice not yet heard
// gets a start grace before it counts as finished; the cap releases the line
// even if the voice never reports done. The gap after the measured words
// covers the voice's leading silence (~65 ms) plus a short pause.
std::string g_compassFollowUp;
DWORD       g_compassFollowUpSince    = 0;
bool        g_compassFollowUpHeard    = false;
DWORD       g_compassFollowUpHeardAt  = 0;
DWORD       g_compassFollowUpSpeechMs = 0;  // 0 = unmeasured: wait for the voice to finish
constexpr DWORD kFollowUpStartGraceMs = 600;
constexpr DWORD kFollowUpMaxWaitMs    = 6000;
constexpr DWORD kFollowUpGapMs        = 150;

int ClampPercent(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

int LoadPercent(PercentSetting& s) {
    if (!s.loaded) {
        s.loaded = true;
        s.value  = ClampPercent(acc::settings::GetInt(s.key, s.value));
    }
    return s.value;
}

void StorePercent(PercentSetting& s, int v) {
    s.loaded = true;
    s.value  = ClampPercent(v);
    acc::settings::SetInt(s.key, s.value);
}

// Push the compass volume and rate to its instance. No-op until created.
void ApplyCompassLevels() {
    if (!g_prismCompass) return;
    const float vol  = static_cast<float>(LoadPercent(g_compassVolume)) / 100.0f;
    const float rate = static_cast<float>(LoadPercent(g_compassRate)) / 100.0f;
    const PrismError vrc = pPrism_backend_set_volume
        ? pPrism_backend_set_volume(g_prismCompass, vol) : kPrismErrNotImplemented;
    const PrismError rrc = pPrism_backend_set_rate
        ? pPrism_backend_set_rate(g_prismCompass, rate) : kPrismErrNotImplemented;
    acclog::Write("Speech", "SAPI compass volume %d%% (rc=%d), rate %d%% (rc=%d)",
                  g_compassVolume.value, vrc, g_compassRate.value, rrc);
}

// Make the compass instance speak with voice `voiceId`. Cached like
// EnsureSapiVoice; false when out of range or SAPI refuses.
bool EnsureCompassVoice(size_t voiceId) {
    if (!g_prismCompass || !pPrism_backend_set_voice) return false;
    if (voiceId >= g_sapiVoiceCount) return false;
    if (voiceId == g_compassVoiceCurrent) return true;
    PrismError rc = pPrism_backend_set_voice(g_prismCompass, voiceId);
    if (rc != kPrismOk) {
        acclog::Trace("Speech",
                      "prism_backend_set_voice(compass, %zu) rc=%d "
                      "(staying on voice %zu)", voiceId, rc, g_compassVoiceCurrent);
        return false;
    }
    g_compassVoiceCurrent = voiceId;
    return true;
}

size_t DefaultCompassVoiceId() {
    const size_t urgent = g_sapiVoiceCurrent;
    if (kCompassDefaultVoiceId < g_sapiVoiceCount && kCompassDefaultVoiceId != urgent)
        return kCompassDefaultVoiceId;
    for (size_t i = 0; i < g_sapiVoiceCount; ++i) {
        if (i != urgent) return i;
    }
    return 0;
}

// The saved voice if it is still installed, else the default. Runs once, after
// the urgent channel has settled its own voice (the default steers around it).
void ApplyCompassVoice() {
    if (g_sapiVoiceCount == 0) return;
    char saved[sizeof(g_sapiVoiceName)] = {0};
    if (acc::settings::GetStr(kCompassVoiceKey, saved, static_cast<int>(sizeof(saved))) &&
        saved[0]) {
        const size_t id = FindSapiVoiceByNameUtf8(saved);
        if (id != static_cast<size_t>(-1)) {
            if (EnsureCompassVoice(id)) {
                acclog::Write("Speech", "SAPI compass voice set to [%s] (id %zu)", saved, id);
            }
            return;
        }
        acclog::Write("Speech",
                      "persisted compass voice [%s] not installed; using the default", saved);
    }
    const size_t id = DefaultCompassVoiceId();
    if (EnsureCompassVoice(id)) {
        acclog::Write("Speech", "SAPI compass voice defaulted to id %zu (urgent is %zu)",
                      id, g_sapiVoiceCurrent);
    }
}

}  // namespace

// Create the compass instance. Needs the urgent SAPI channel up first: it
// supplies the voice list and the urgent voice the default steers around.
bool TryCreateCompass() {
    if (!g_sapiReady || !pPrism_registry_create) {
        acclog::Write("Speech",
                      "compass channel unavailable; the compass speaks on the "
                      "announcement voice");
        return false;
    }
    DWORD seh = 0;
    PrismBackend* be = SehCreate(g_prismCtx, kPrismBackendIdSapi, &seh);
    PrismError rc = be ? SehInitialize(be, &seh) : kPrismErrUnknown;
    if (!be || seh || (rc != kPrismOk && rc != kPrismErrAlreadyInitialized)) {
        acclog::Write("Speech",
                      "compass channel failed (rc=%d seh=0x%08lX); the compass "
                      "speaks on the announcement voice", rc, seh);
        return false;
    }
    g_prismCompass = be;
    if (pPrism_backend_get_voice) {
        size_t cur = 0;
        if (pPrism_backend_get_voice(g_prismCompass, &cur) == kPrismOk) {
            g_compassVoiceCurrent = cur;
        }
    }
    ApplyCompassLevels();
    ApplyCompassVoice();
    acclog::Write("Speech", "compass channel ready (second SAPI instance)");
    return true;
}

// Forget a waiting follow-up: newer speech has made it stale.
void DropCompassFollowUp(const char* why) {
    if (g_compassFollowUp.empty()) return;
    acclog::Trace("Speech", "compass follow-up dropped (%s)", why);
    g_compassFollowUp.clear();
}

// The compass instance's counterpart of SpeakUrgentImpl: always interrupts,
// mirrors to braille, never falls back to another voice on a dispatch error
// (SAPI may already be mid-utterance — same double-speak reasoning).
void SpeakCompass(const char* text) {
    DropCompassFollowUp("newer compass line");
    if (AllViaScreenReader()) {
        Speak(text, /*interrupt=*/true);
        return;
    }
    if (!g_compassReady) {
        SpeakUrgentImpl(text, /*interrupt=*/true);
        return;
    }
    if (!g_normalReady || !text || !*text) return;
    CaptureSpoken(text);
    std::vector<char> utf8(strlen(text) * 4 + 1);
    const char* speakText = ReencodeAcpToUtf8(text, utf8.data(), utf8.size()) ? utf8.data() : text;
    PrismError rc = pPrism_backend_speak(g_prismCompass, speakText, /*interrupt=*/true);
    BrailleNormal(speakText);
    if (rc == kPrismOk) {
        acclog::Trace("Speech.spoke", "[compass] %s", text);
    } else {
        acclog::Trace("Speech", "SpeakCompass: dispatch rc=%d; dropping utterance", rc);
    }
}

// How long the compass voice takes to say `text`, silence trimmed (renders
// come back gated); 0 when it cannot be measured. A render costs one game
// frame at most (0-31 ms logged for lines this short).
static DWORD CompassSpeechMs(const char* text) {
    char voice[sizeof(g_sapiVoiceName)] = {};
    if (!FetchSapiVoiceNameUtf8(g_compassVoiceCurrent, voice, sizeof(voice))) return 0;
    RenderedSpeech r;
    if (!RenderOn(g_renderSapi, voice, RateFromPercent(LoadPercent(g_compassRate)), 1.0f, text, r))
        return 0;
    return static_cast<DWORD>(r.samples.size() / r.channels * 1000 / r.sampleRate);
}

void SpeakCompassThen(const char* text, const char* followUp) {
    if (AllViaScreenReader() && text && *text) {
        if (followUp && *followUp) {
            const std::string both = std::string(text) + "\n" + followUp;
            Speak(both.c_str(), /*interrupt=*/true);
        } else {
            Speak(text, /*interrupt=*/true);
        }
        return;
    }
    if (!followUp || !*followUp) {
        SpeakCompass(text);
        return;
    }
    if (!g_normalReady || !text || !*text) return;
    const std::string line = std::string(text) + "\n" + followUp;
    if (!g_compassReady || !pPrism_backend_is_speaking) {
        // No second voice to take turns with: one line on the screen reader,
        // as before the compass voice existed.
        Speak(line.c_str(), /*interrupt=*/true);
        return;
    }
    const DWORD speechMs = CompassSpeechMs(text);
    // The follow-up lands on the screen reader; clear it now so the compass
    // line does not play over something stale still being read there.
    (void)pPrism_backend_stop(g_prismNormal);
    SpeakCompass(text);
    // Ctrl+R copies the whole readout, not just the half spoken last.
    CaptureSpoken(line.c_str());
    g_compassFollowUp         = followUp;
    g_compassFollowUpSince    = GetTickCount();
    g_compassFollowUpHeard    = false;
    g_compassFollowUpSpeechMs = speechMs;
}

void Tick() {
    if (g_compassFollowUp.empty()) return;
    const DWORD now    = GetTickCount();
    const DWORD waited = now - g_compassFollowUpSince;
    const char* why    = "wait cap";
    if (waited < kFollowUpMaxWaitMs) {
        bool speaking = false;
        if (pPrism_backend_is_speaking(g_prismCompass, &speaking) == kPrismOk && speaking) {
            if (!g_compassFollowUpHeard) {
                g_compassFollowUpHeard   = true;
                g_compassFollowUpHeardAt = now;
            }
            // The voice still counts as speaking through the silence it pads
            // every line with (~380 ms for Stefan); with the words measured,
            // the follow-up goes once they are over instead.
            if (g_compassFollowUpSpeechMs == 0 ||
                now - g_compassFollowUpHeardAt < g_compassFollowUpSpeechMs + kFollowUpGapMs)
                return;
            why = "compass words over";
        } else if (!g_compassFollowUpHeard && waited < kFollowUpStartGraceMs) {
            return;
        } else {
            why = g_compassFollowUpHeard ? "compass voice finished" : "compass voice never started";
        }
    }
    acclog::Trace("Speech", "compass follow-up released after %lu ms (%s, words %lu ms)", waited,
                  why, g_compassFollowUpSpeechMs);
    std::string followUp;
    followUp.swap(g_compassFollowUp);
    s_suppressNextCapture = true;  // LastSpoken already holds the whole line
    Speak(followUp.c_str(), /*interrupt=*/false);
}

void SetCompassVolumePercent(int percent) {
    StorePercent(g_compassVolume, percent);
    ApplyCompassLevels();
}

int GetCompassVolumePercent() {
    return LoadPercent(g_compassVolume);
}

void SetCompassRatePercent(int percent) {
    StorePercent(g_compassRate, percent);
    ApplyCompassLevels();
}

int GetCompassRatePercent() {
    return LoadPercent(g_compassRate);
}

size_t CompassVoiceCount() {
    return g_compassReady ? g_sapiVoiceCount : 0;
}

size_t GetCompassVoiceId() {
    if (g_compassVoiceCurrent >= g_sapiVoiceCount) return 0;
    return g_compassVoiceCurrent;
}

void SetCompassVoiceId(size_t voiceId) {
    if (!g_compassReady || voiceId >= g_sapiVoiceCount) return;
    // Cut the sample still playing first — see SetUrgentVoiceId.
    if (pPrism_backend_stop) pPrism_backend_stop(g_prismCompass);
    if (!EnsureCompassVoice(voiceId)) return;
    char utf8[sizeof(g_sapiVoiceName)];
    if (!FetchSapiVoiceNameUtf8(voiceId, utf8, sizeof(utf8))) return;
    acc::settings::SetStr(kCompassVoiceKey, utf8);
    acclog::Write("Speech", "SAPI compass voice -> [%s] (id %zu)", utf8, voiceId);
}

}  // namespace prism
