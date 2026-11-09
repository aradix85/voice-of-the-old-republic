// The urgent channel: a SAPI backend that bypasses the screen reader's
// typed-character cancel. Its voice, volume and rate persist in the mod settings.

#include "prism_internal.h"

namespace prism {

namespace {

// User-facing SAPI urgent-channel volume, percent [0,100]; 100 = SAPI full.
// Backed by the persistent store (acc_settings.ini) so it survives relaunch,
// lazily pulled on first access. Prism maps the float arg of set_volume to
// [0.0..1.0] (same convention as set_rate), so the byte we hand over is
// percent/100. The slider in Mod-Einstellungen drives this via
// SetUrgentVolumePercent. (ApplySapiVolume lives below the function-pointer
// block — it needs pPrism_backend_set_volume.)
constexpr const char* kSapiVolumeKey   = "UrgentVolumePercent";
int  g_sapiVolumePercent = 100;
bool g_sapiVolumeLoaded  = false;

void EnsureSapiVoiceLoaded() {
    if (g_sapiVoiceLoaded) return;
    g_sapiVoiceLoaded = true;
    if (!acc::settings::GetStr(kSapiVoiceKey, g_sapiVoiceName,
                               static_cast<int>(sizeof(g_sapiVoiceName)))) {
        g_sapiVoiceName[0] = '\0';
    }
}

void EnsureSapiVolumeLoaded() {
    if (g_sapiVolumeLoaded) return;
    g_sapiVolumeLoaded = true;
    int v = acc::settings::GetInt(kSapiVolumeKey, 100);
    if (v < 0)   v = 0;
    if (v > 100) v = 100;
    g_sapiVolumePercent = v;
}

// Push g_sapiVolumePercent to the live SAPI backend. Safe to call any time;
// no-ops if the backend isn't acquired yet or the export is missing (older
// Prism builds / backends without SUPPORTS_SET_VOLUME). Does NOT gate on
// g_sapiReady so it can run mid-acquire (when g_prismSapi is set but the
// ready flag hasn't flipped).
void ApplySapiVolume() {
    if (!g_prismSapi || !pPrism_backend_set_volume) return;
    EnsureSapiVolumeLoaded();
    float vol = static_cast<float>(g_sapiVolumePercent) / 100.0f;
    PrismError rc = pPrism_backend_set_volume(g_prismSapi, vol);
    if (rc != kPrismOk) {
        acclog::Write("Speech",
                      "prism_backend_set_volume(SAPI, %.2f) rc=%d "
                      "(SAPI staying at default volume)", vol, rc);
    } else {
        acclog::Write("Speech",
                      "SAPI urgent volume set to %d%% (%.2f, 0.0=mute, "
                      "1.0=full)", g_sapiVolumePercent, vol);
    }
}

// Push g_sapiRatePercent to the live SAPI backend. Mirror of ApplySapiVolume,
// including the "safe any time, no-op until acquired" contract: older Prism
// builds without prism_backend_set_rate simply leave SAPI at its own rate.
void ApplySapiRate() {
    if (!g_prismSapi || !pPrism_backend_set_rate) return;
    EnsureSapiRateLoaded();
    float rate = static_cast<float>(g_sapiRatePercent) / 100.0f;
    PrismError rc = pPrism_backend_set_rate(g_prismSapi, rate);
    if (rc != kPrismOk) {
        acclog::Write("Speech",
                      "prism_backend_set_rate(SAPI, %.2f) rc=%d "
                      "(SAPI staying at default rate)", rate, rc);
    } else {
        acclog::Write("Speech",
                      "SAPI urgent rate set to %d%% (%.2f, 0.0=slowest, "
                      "0.5=SAPI default, 1.0=fastest)",
                      g_sapiRatePercent, rate);
    }
}

// Make the SAPI backend speak with voice `voiceId`. Cached so consecutive
// calls with the same id are free. Out-of-range / unsupported silently no-op
// and leave the current voice in place. Returns false only on a hard set_voice
// error so the caller can log; the speak still proceeds with whatever voice
// SAPI currently has. Gated on g_prismSapi rather than g_sapiReady, like
// ApplySapiVolume, so the persisted choice can be applied mid-acquire.
bool EnsureSapiVoice(size_t voiceId) {
    if (!g_prismSapi || !pPrism_backend_set_voice) return false;
    if (g_sapiVoiceCount != 0 && voiceId >= g_sapiVoiceCount) return false;
    if (voiceId == g_sapiVoiceCurrent) return true;
    PrismError rc = pPrism_backend_set_voice(g_prismSapi, voiceId);
    if (rc != kPrismOk) {
        acclog::Trace("Speech",
                      "prism_backend_set_voice(SAPI, %zu) rc=%d "
                      "(staying on voice %zu)", voiceId, rc, g_sapiVoiceCurrent);
        return false;
    }
    g_sapiVoiceCurrent = voiceId;
    return true;
}

// Apply the persisted voice choice to the freshly-acquired backend. Runs once
// per launch from TryAcquireSapi, after the voice count is known. A name that
// no longer resolves is left in the store rather than cleared — the voice may
// come back on the next launch, and silently forgetting the user's choice is
// worse than one log line.
void ApplySapiVoice() {
    EnsureSapiVoiceLoaded();
    if (!g_sapiVoiceName[0]) return;
    size_t id = FindSapiVoiceByNameUtf8(g_sapiVoiceName);
    if (id == static_cast<size_t>(-1)) {
        acclog::Write("Speech",
                      "persisted SAPI voice [%s] not installed; "
                      "staying on the default voice", g_sapiVoiceName);
        return;
    }
    if (EnsureSapiVoice(id)) {
        acclog::Write("Speech", "SAPI urgent voice set to [%s] (id %zu)",
                      g_sapiVoiceName, id);
    }
}

}  // namespace

void EnsureSapiRateLoaded() {
    if (g_sapiRateLoaded) return;
    g_sapiRateLoaded = true;
    int v = acc::settings::GetInt(kSapiRateKey, kSapiRateDefaultPercent);
    if (v < 0)   v = 0;
    if (v > 100) v = 100;
    g_sapiRatePercent = v;
}

// Copy voice `voiceId`'s name into outBuf as UTF-8 — Prism's own encoding,
// and what the persisted setting stores. The pointer Prism hands back aliases
// a backend-owned string that the NEXT get_voice_name call overwrites, so the
// copy has to happen before anything else asks for a name.
bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize) {
    if (!g_prismSapi || !pPrism_backend_get_voice_name) return false;
    if (!outBuf || bufSize == 0) return false;
    if (g_sapiVoiceCount != 0 && voiceId >= g_sapiVoiceCount) return false;
    const char* name = nullptr;
    PrismError rc = pPrism_backend_get_voice_name(g_prismSapi, voiceId, &name);
    if (rc != kPrismOk || !name) return false;
    snprintf(outBuf, bufSize, "%s", name);
    return outBuf[0] != '\0';
}

// Index of the voice named `utf8Name`, or SIZE_MAX when the machine no longer
// has it (uninstalled voice, moved save file, language pack removed).
size_t FindSapiVoiceByNameUtf8(const char* utf8Name) {
    if (!utf8Name || !*utf8Name) return static_cast<size_t>(-1);
    char probe[sizeof(g_sapiVoiceName)];
    for (size_t i = 0; i < g_sapiVoiceCount; ++i) {
        if (!FetchSapiVoiceNameUtf8(i, probe, sizeof(probe))) continue;
        if (strcmp(probe, utf8Name) == 0) return i;
    }
    return static_cast<size_t>(-1);
}

// Acquire the SAPI backend explicitly and bump its rate. Returns true iff the
// backend is fully initialised and ready to receive prism_backend_speak.
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
    PrismError rc = SehInitialize(g_prismSapi, &seh);
    // acquire_best initialises eagerly; explicit acquire requires initialize().
    // ALREADY_INITIALIZED is non-fatal — we just got a cached instance.
    if (seh) {
        acclog::Write("Speech",
                      "prism_backend_initialize(SAPI) faulted (%s 0x%08lX)",
                      SehCodeName(seh), seh);
        g_prismSapi = nullptr;
        return false;
    }
    if (rc != kPrismOk && rc != kPrismErrAlreadyInitialized) {
        acclog::Write("Speech",
                      "prism_backend_initialize(SAPI) failed rc=%d", rc);
        g_prismSapi = nullptr;
        return false;
    }

    // Apply the persisted urgent-channel sliders to the freshly-acquired
    // backend. The Ensure*Loaded helpers pull the saved values (defaults:
    // kSapiRateDefaultPercent rate, 100% volume) so the user's last choice
    // carries across launches.
    ApplySapiRate();
    ApplySapiVolume();

    // Cache voice count so the voice picker and every range check can work
    // off it — cheaper than a round trip per query.
    if (pPrism_backend_count_voices) {
        size_t count = 0;
        PrismError vrc = pPrism_backend_count_voices(g_prismSapi, &count);
        if (vrc == kPrismOk) {
            g_sapiVoiceCount = count;
            acclog::Write("Speech", "SAPI voice count = %zu", count);
        } else {
            acclog::Write("Speech",
                          "prism_backend_count_voices(SAPI) rc=%d "
                          "(voice selection unavailable)",
                          vrc);
        }
    }

    // Seed the cached index from whatever voice SAPI is already on, so the
    // picker opens on the real current voice instead of guessing 0 — and so
    // EnsureSapiVoice's "already there" short-circuit tells the truth.
    if (pPrism_backend_get_voice) {
        size_t cur = 0;
        if (pPrism_backend_get_voice(g_prismSapi, &cur) == kPrismOk) {
            g_sapiVoiceCurrent = cur;
        }
    }

    // Then honour the user's persisted choice, if they have made one.
    ApplySapiVoice();
    return true;
}

void SetUrgentVolumePercent(int percent) {
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    g_sapiVolumeLoaded  = true;  // we are the authoritative value now
    g_sapiVolumePercent = percent;
    acc::settings::SetInt(kSapiVolumeKey, percent);  // persist across launches
    ApplySapiVolume();  // push to the live backend (no-op if not ready)
}

int GetUrgentVolumePercent() {
    EnsureSapiVolumeLoaded();
    return g_sapiVolumePercent;
}

void SetUrgentRatePercent(int percent) {
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    g_sapiRateLoaded  = true;  // we are the authoritative value now
    g_sapiRatePercent = percent;
    acc::settings::SetInt(kSapiRateKey, percent);  // persist across launches
    ApplySapiRate();  // push to the live backend (no-op if not ready)
}

int GetUrgentRatePercent() {
    EnsureSapiRateLoaded();
    return g_sapiRatePercent;
}

size_t UrgentVoiceCount() {
    return g_sapiVoiceCount;
}

bool GetUrgentVoiceName(size_t voiceId, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    char utf8[sizeof(g_sapiVoiceName)];
    if (!FetchSapiVoiceNameUtf8(voiceId, utf8, sizeof(utf8))) return false;
    if (!ReencodeUtf8ToGameCodepage(utf8, outBuf, bufSize)) {
        // Non-representable characters in the player's codepage — the raw
        // UTF-8 still names the voice recognisably for anything ASCII, which
        // every shipped Microsoft voice name is.
        snprintf(outBuf, bufSize, "%s", utf8);
    }
    return outBuf[0] != '\0';
}

size_t GetUrgentVoiceId() {
    if (g_sapiVoiceCount == 0) return 0;
    if (g_sapiVoiceCurrent >= g_sapiVoiceCount) return 0;
    return g_sapiVoiceCurrent;
}

void SetUrgentVoiceId(size_t voiceId) {
    if (voiceId >= g_sapiVoiceCount) return;
    // Cut any sample still playing before the swap. SAPI re-queries the
    // output format as part of SetVoice, and doing that against a device
    // mid-utterance is the one way this call fails — which would leave the
    // picker reading the old voice back and looking stuck under a fast
    // Left/Right walk. The caller speaks a fresh sample straight after, so
    // there is nothing worth preserving.
    if (g_prismSapi && pPrism_backend_stop) pPrism_backend_stop(g_prismSapi);
    if (!EnsureSapiVoice(voiceId)) return;
    // Persist by name, not by index — see kSapiVoiceKey.
    EnsureSapiVoiceLoaded();
    char utf8[sizeof(g_sapiVoiceName)];
    if (!FetchSapiVoiceNameUtf8(voiceId, utf8, sizeof(utf8))) return;
    snprintf(g_sapiVoiceName, sizeof(g_sapiVoiceName), "%s", utf8);
    acc::settings::SetStr(kSapiVoiceKey, g_sapiVoiceName);
    acclog::Write("Speech", "SAPI urgent voice -> [%s] (id %zu)",
                  g_sapiVoiceName, voiceId);
}

}  // namespace prism
