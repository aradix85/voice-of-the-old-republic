// The urgent channel: a SAPI backend that bypasses the screen reader's
// typed-character cancel. Its voice, volume and rate persist in the mod
// settings; the helpers for persisted levels and voices serve the compass
// channel too.

#include "prism_internal.h"

#include "log.h"
#include "mod_settings_store.h"

namespace prism {

namespace {

// Urgent-channel volume; 100 = SAPI full. Only this channel is affected; the
// screen reader keeps the user's own volume.
PercentSetting g_sapiVolume = {"UrgentVolumePercent", 100, false};

// The urgent channel's voice, persisted BY NAME rather than by index: SAPI
// numbers voices in enumeration order, so installing or removing any voice
// renumbers the rest and a saved index would land the user on a different
// voice — quite possibly one speaking a different language. The name is
// stored in Prism's own encoding (UTF-8), which is what the id lookup
// compares against. Empty = never chosen; the channel keeps SAPI's default.
constexpr const char* kSapiVoiceKey = "UrgentVoiceName";
char g_sapiVoiceName[kVoiceNameMax] = {0};
bool g_sapiVoiceLoaded              = false;

void EnsureSapiVoiceLoaded() {
    if (g_sapiVoiceLoaded) return;
    g_sapiVoiceLoaded = true;
    if (!acc::settings::GetStr(kSapiVoiceKey, g_sapiVoiceName,
                               static_cast<int>(sizeof(g_sapiVoiceName)))) {
        g_sapiVoiceName[0] = '\0';
    }
}

// Apply the persisted voice choice to the freshly-acquired backend. A name
// that no longer resolves is left in the store rather than cleared — the voice
// may come back on the next launch, and silently forgetting the user's choice
// is worse than one log line.
void ApplySapiVoice() {
    EnsureSapiVoiceLoaded();
    if (!g_sapiVoiceName[0]) return;
    const size_t id = FindSapiVoiceByNameUtf8(g_sapiVoiceName);
    if (id == kNoVoice) {
        acclog::Write("Speech",
                      "persisted SAPI voice [%s] not installed; "
                      "staying on the default voice", g_sapiVoiceName);
        return;
    }
    if (SetVoiceCached(g_prismSapi, g_sapiVoiceCurrent, id, "SAPI")) {
        acclog::Write("Speech", "SAPI urgent voice set to [%s] (id %zu)",
                      g_sapiVoiceName, id);
    }
}

void ApplyUrgentLevels() {
    ApplyLevels(g_prismSapi, g_sapiVolume, g_sapiRate, "urgent");
}

}  // namespace

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

// Push a channel's volume and rate to its backend; Prism takes both as
// 0.0..1.0. No-op until the backend exists.
void ApplyLevels(PrismBackend* be, PercentSetting& volume, PercentSetting& rate,
                 const char* channel) {
    if (!be) return;
    const PrismError vrc = pPrism_backend_set_volume(be, LoadPercent(volume) / 100.0f);
    const PrismError rrc = pPrism_backend_set_rate(be, LoadPercent(rate) / 100.0f);
    acclog::Write("Speech", "SAPI %s volume %d%% (rc=%d), rate %d%% (rc=%d)",
                  channel, volume.value, vrc, rate.value, rrc);
}

// Make `be` speak with voice `voiceId`, remembered in `current` so a repeat
// is free. False when out of range or SAPI refuses; the voice then stays.
bool SetVoiceCached(PrismBackend* be, size_t& current, size_t voiceId, const char* channel) {
    if (!be) return false;
    if (g_sapiVoiceCount != 0 && voiceId >= g_sapiVoiceCount) return false;
    if (voiceId == current) return true;
    const PrismError rc = pPrism_backend_set_voice(be, voiceId);
    if (rc != kPrismOk) {
        acclog::Trace("Speech",
                      "prism_backend_set_voice(%s, %zu) rc=%d (staying on voice %zu)",
                      channel, voiceId, rc, current);
        return false;
    }
    current = voiceId;
    return true;
}

// Copy voice `voiceId`'s name into outBuf as UTF-8 — Prism's own encoding,
// and what the persisted setting stores. The pointer Prism hands back aliases
// a backend-owned string that the NEXT get_voice_name call overwrites, so the
// copy has to happen before anything else asks for a name.
bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize) {
    if (!g_prismSapi) return false;
    if (g_sapiVoiceCount != 0 && voiceId >= g_sapiVoiceCount) return false;
    const char* name = nullptr;
    PrismError rc = pPrism_backend_get_voice_name(g_prismSapi, voiceId, &name);
    if (rc != kPrismOk || !name) return false;
    snprintf(outBuf, bufSize, "%s", name);
    return outBuf[0] != '\0';
}

// Index of the voice named `utf8Name`, or kNoVoice when the machine no longer
// has it (uninstalled voice, moved settings file, language pack removed).
size_t FindSapiVoiceByNameUtf8(const char* utf8Name) {
    char probe[kVoiceNameMax];
    for (size_t i = 0; i < g_sapiVoiceCount; ++i) {
        if (!FetchSapiVoiceNameUtf8(i, probe, sizeof(probe))) continue;
        if (strcmp(probe, utf8Name) == 0) return i;
    }
    return kNoVoice;
}

// Acquire the SAPI backend explicitly. Returns true iff the backend is fully
// initialised and ready to receive prism_backend_speak.
bool TryAcquireSapi() {
    if (!pPrism_registry_exists(g_prismCtx, kPrismBackendIdSapi)) {
        acclog::Write("Speech",
                      "SAPI backend not in registry; SpeakUrgent will fall "
                      "back to the normal backend");
        return false;
    }
    DWORD seh = 0;
    g_prismSapi = SehAcquire(g_prismCtx, kPrismBackendIdSapi, &seh);
    if (!g_prismSapi) {
        if (seh) {
            acclog::Write("Speech",
                          "prism_registry_acquire(SAPI) faulted (%s 0x%08lX)",
                          SehCodeName(seh), seh);
        } else {
            acclog::Write("Speech", "prism_registry_acquire(SAPI) returned NULL");
        }
        return false;
    }
    // acquire_best initialises eagerly; explicit acquire requires initialize().
    PrismError rc = SehInitialize(g_prismSapi, &seh);
    if (seh) {
        acclog::Write("Speech",
                      "prism_backend_initialize(SAPI) faulted (%s 0x%08lX)",
                      SehCodeName(seh), seh);
        g_prismSapi = nullptr;
        return false;
    }
    if (!InitOk(rc)) {
        acclog::Write("Speech",
                      "prism_backend_initialize(SAPI) failed rc=%d", rc);
        g_prismSapi = nullptr;
        return false;
    }

    ApplyUrgentLevels();

    // Cache the voice count so the voice picker and every range check can
    // work off it — cheaper than a round trip per query.
    size_t count = 0;
    PrismError vrc = pPrism_backend_count_voices(g_prismSapi, &count);
    if (vrc == kPrismOk) {
        g_sapiVoiceCount = count;
        acclog::Write("Speech", "SAPI voice count = %zu", count);
    } else {
        acclog::Write("Speech",
                      "prism_backend_count_voices(SAPI) rc=%d "
                      "(voice selection unavailable)", vrc);
    }

    // Seed the cached index from whatever voice SAPI is already on, so the
    // picker opens on the real current voice instead of guessing 0 — and so
    // SetVoiceCached's "already there" short-circuit tells the truth.
    size_t cur = 0;
    if (pPrism_backend_get_voice(g_prismSapi, &cur) == kPrismOk) g_sapiVoiceCurrent = cur;

    ApplySapiVoice();
    return true;
}

void SetUrgentVolumePercent(int percent) {
    StorePercent(g_sapiVolume, percent);
    ApplyUrgentLevels();
}

int GetUrgentVolumePercent() {
    return LoadPercent(g_sapiVolume);
}

void SetUrgentRatePercent(int percent) {
    StorePercent(g_sapiRate, percent);
    ApplyUrgentLevels();
}

int GetUrgentRatePercent() {
    return LoadPercent(g_sapiRate);
}

size_t UrgentVoiceCount() {
    return g_sapiVoiceCount;
}

bool GetUrgentVoiceName(size_t voiceId, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    char utf8[kVoiceNameMax];
    if (!FetchSapiVoiceNameUtf8(voiceId, utf8, sizeof(utf8))) return false;
    // A name the game codepage cannot hold stays UTF-8: still recognisable for
    // anything ASCII, which every shipped Microsoft voice name is.
    const std::string name = FromUtf8(utf8);
    snprintf(outBuf, bufSize, "%s", name.empty() ? utf8 : name.c_str());
    return outBuf[0] != '\0';
}

size_t GetUrgentVoiceId() {
    return g_sapiVoiceCurrent < g_sapiVoiceCount ? g_sapiVoiceCurrent : 0;
}

void SetUrgentVoiceId(size_t voiceId) {
    if (voiceId >= g_sapiVoiceCount) return;
    // Cut any sample still playing before the swap. SAPI re-queries the
    // output format as part of SetVoice, and doing that against a device
    // mid-utterance is the one way this call fails — which would leave the
    // picker reading the old voice back and looking stuck under a fast
    // Left/Right walk. The caller speaks a fresh sample straight after, so
    // there is nothing worth preserving.
    pPrism_backend_stop(g_prismSapi);
    if (!SetVoiceCached(g_prismSapi, g_sapiVoiceCurrent, voiceId, "SAPI")) return;
    // Persist by name, not by index — see kSapiVoiceKey.
    EnsureSapiVoiceLoaded();
    char utf8[kVoiceNameMax];
    if (!FetchSapiVoiceNameUtf8(voiceId, utf8, sizeof(utf8))) return;
    snprintf(g_sapiVoiceName, sizeof(g_sapiVoiceName), "%s", utf8);
    acc::settings::SetStr(kSapiVoiceKey, g_sapiVoiceName);
    acclog::Write("Speech", "SAPI urgent voice -> [%s] (id %zu)",
                  g_sapiVoiceName, voiceId);
}

}  // namespace prism
