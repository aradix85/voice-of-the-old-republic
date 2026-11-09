// The compass channel: a second SAPI instance with its own voice, volume and
// rate for the camera-turn readout, and the follow-up line spoken on the normal
// channel once the compass voice has finished.
//
// A voice of its own lets the player tell a heading from an announcement by
// the voice alone, and neither cuts the other off — each instance owns its own
// SpVoice and audio stream. prism_registry_create builds it fresh;
// prism_registry_acquire would hand back the urgent channel's cached one.
//
// Volume, rate and voice persist like the urgent channel's, the voice by name.
// Without a saved voice the channel takes kCompassDefaultVoiceId, or the first
// voice that is not the urgent one when that id is missing or is the urgent
// voice itself: the default only has to sound different from the
// announcements. It is re-derived each launch until the player chooses one.

#include "prism_internal.h"

#include "log.h"
#include "mod_settings_store.h"

namespace prism {

namespace {

constexpr size_t      kCompassDefaultVoiceId = 2;
constexpr const char* kCompassVoiceKey       = "CompassVoiceName";

size_t         g_compassVoiceCurrent = kNoVoice;
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

void ApplyCompassLevels() {
    ApplyLevels(g_prismCompass, g_compassVolume, g_compassRate, "compass");
}

bool SetCompassVoice(size_t voiceId) {
    return SetVoiceCached(g_prismCompass, g_compassVoiceCurrent, voiceId, "compass");
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
    char saved[kVoiceNameMax] = {0};
    if (acc::settings::GetStr(kCompassVoiceKey, saved, static_cast<int>(sizeof(saved))) &&
        saved[0]) {
        const size_t id = FindSapiVoiceByNameUtf8(saved);
        if (id != kNoVoice) {
            if (SetCompassVoice(id)) {
                acclog::Write("Speech", "SAPI compass voice set to [%s] (id %zu)", saved, id);
            }
            return;
        }
        acclog::Write("Speech",
                      "persisted compass voice [%s] not installed; using the default", saved);
    }
    const size_t id = DefaultCompassVoiceId();
    if (SetCompassVoice(id)) {
        acclog::Write("Speech", "SAPI compass voice defaulted to id %zu (urgent is %zu)",
                      id, g_sapiVoiceCurrent);
    }
}

// How long the compass voice takes to say `text`, silence trimmed (renders
// come back gated); 0 when it cannot be measured. A render costs one game
// frame at most (0-31 ms logged for lines this short).
DWORD CompassSpeechMs(const char* text) {
    char voice[kVoiceNameMax] = {};
    if (!FetchSapiVoiceNameUtf8(g_compassVoiceCurrent, voice, sizeof(voice))) return 0;
    RenderedSpeech r;
    if (!RenderSapiVoiceToMemory(voice, LoadPercent(g_compassRate), text, r)) return 0;
    return static_cast<DWORD>(r.samples.size() / r.channels * 1000 / r.sampleRate);
}

}  // namespace

// Create the compass instance. Needs the urgent SAPI channel up first: it
// supplies the voice list and the urgent voice the default steers around.
bool TryCreateCompass() {
    if (!g_sapiReady) {
        acclog::Write("Speech",
                      "compass channel unavailable; the compass speaks on the "
                      "announcement voice");
        return false;
    }
    PrismError rc = kPrismOk;
    DWORD seh = 0;
    PrismBackend* be = CreateBackend(kPrismBackendIdSapi, &rc, &seh);
    if (!be) {
        acclog::Write("Speech",
                      "compass channel failed (rc=%d seh=0x%08lX); the compass "
                      "speaks on the announcement voice", rc, seh);
        return false;
    }
    g_prismCompass = be;
    size_t cur = 0;
    if (pPrism_backend_get_voice(g_prismCompass, &cur) == kPrismOk) g_compassVoiceCurrent = cur;
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
    const std::string utf8 = ToUtf8(text);
    const char* speakText = utf8.empty() ? text : utf8.c_str();
    PrismError rc = pPrism_backend_speak(g_prismCompass, speakText, /*interrupt=*/true);
    BrailleNormal(speakText);
    if (rc == kPrismOk) {
        acclog::Trace("Speech.spoke", "[compass] %s", text);
    } else {
        acclog::Trace("Speech", "SpeakCompass: dispatch rc=%d; dropping utterance", rc);
    }
}

void SpeakCompassThen(const char* text, const char* followUp) {
    if (!followUp || !*followUp) {
        SpeakCompass(text);
        return;
    }
    if (!text || !*text) return;
    const std::string line = std::string(text) + "\n" + followUp;
    if (AllViaScreenReader() || !g_compassReady) {
        // No second voice to take turns with: one line on the screen reader.
        Speak(line.c_str(), /*interrupt=*/true);
        return;
    }
    if (!g_normalReady) return;
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
    SuppressNextCapture();  // LastSpoken already holds the whole line
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
    return g_compassVoiceCurrent < g_sapiVoiceCount ? g_compassVoiceCurrent : 0;
}

void SetCompassVoiceId(size_t voiceId) {
    if (!g_compassReady || voiceId >= g_sapiVoiceCount) return;
    // Cut the sample still playing first — see SetUrgentVoiceId.
    pPrism_backend_stop(g_prismCompass);
    if (!SetCompassVoice(voiceId)) return;
    char utf8[kVoiceNameMax];
    if (!FetchSapiVoiceNameUtf8(voiceId, utf8, sizeof(utf8))) return;
    acc::settings::SetStr(kCompassVoiceKey, utf8);
    acclog::Write("Speech", "SAPI compass voice -> [%s] (id %zu)", utf8, voiceId);
}

}  // namespace prism
