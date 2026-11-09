// Speech rendered to memory (PCM) instead of spoken, on render-only SAPI and
// OneCore instances: for positional speech and for timing a line.

#include "prism_internal.h"

#include "log.h"

namespace prism {

namespace {

// FNV-1a of "OneCore", Prism's backend_catalog.h.
constexpr uint64_t kPrismBackendIdOneCore = 0x6797D32F0D994CB4ull;

// Render-only backends: separate instances that never speak aloud. A render on
// the urgent channel's own SAPI voice would queue behind (and block the game
// on) whatever that voice is still saying — SAPI's speak-to-memory is a
// synchronous Speak on the same SpVoice. Created on first use, on the game
// thread, and only ever used there.
struct RenderBackend {
    const char*   tag;
    uint64_t      id;
    PrismBackend* be;
    bool          tried;
    char          voice[kVoiceNameMax];   // last voice selected, to log only changes
};
RenderBackend g_renderSapi    = {"SAPI",    kPrismBackendIdSapi,    nullptr, false, {}};
RenderBackend g_renderOneCore = {"OneCore", kPrismBackendIdOneCore, nullptr, false, {}};

// Render one utterance to memory, SEH-guarded. Same contract as SehInitialize.
PrismError SehSpeakToMemory(PrismBackend* be, const char* utf8, PFN_PrismAudioCallback cb,
                            void* userdata, DWORD* outCode) {
    *outCode = 0;
    __try {
        return pPrism_backend_speak_to_memory(be, utf8, cb, userdata);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *outCode = GetExceptionCode();
        return kPrismErrUnknown;
    }
}

// Copies the rendered block out of Prism's buffer, which is only valid for
// the duration of the callback.
void __cdecl CollectRenderedSpeech(void* userdata, const float* samples, size_t sampleCount,
                                   size_t channels, size_t sampleRate) {
    auto* out = static_cast<RenderedSpeech*>(userdata);
    out->samples.assign(samples, samples + sampleCount);
    out->channels   = channels;
    out->sampleRate = sampleRate;
}

PrismBackend* EnsureRenderBackend(RenderBackend& r) {
    if (r.tried) return r.be;
    r.tried = true;
    if (!g_prismCtx || !pPrism_registry_exists(g_prismCtx, r.id)) {
        acclog::Write("Speech", "render[%s] backend: unavailable", r.tag);
        return nullptr;
    }
    PrismError rc = kPrismOk;
    DWORD seh = 0;
    r.be = CreateBackend(r.id, &rc, &seh);
    acclog::Write("Speech", "render[%s] backend: %s (rc=%d seh=0x%08lX)", r.tag,
                  r.be ? "ready" : "failed", rc, seh);
    return r.be;
}

// Does Prism's voice name `name` denote the voice described as `wanted`?
// Equal, or `wanted` begins with `name` + " ": OneCore lists "Microsoft
// Stefan" for the registry token whose display string is "Microsoft Stefan -
// German (Germany)", while SAPI names match the token string exactly.
bool VoiceNameMatches(const char* name, const char* wanted) {
    const size_t n = strlen(name);
    if (n == 0 || strncmp(name, wanted, n) != 0) return false;
    return wanted[n] == '\0' || wanted[n] == ' ';
}

// Point `r` at the first voice matching `wanted` (UTF-8). Voices are backend
// state, so this runs every render; a handful of names, cheap.
bool SelectVoice(RenderBackend& r, const char* wanted) {
    size_t count = 0;
    if (pPrism_backend_count_voices(r.be, &count) != kPrismOk) return false;
    for (size_t i = 0; i < count; ++i) {
        const char* name = nullptr;
        if (pPrism_backend_get_voice_name(r.be, i, &name) != kPrismOk || !name) continue;
        if (!VoiceNameMatches(name, wanted)) continue;
        if (pPrism_backend_set_voice(r.be, i) != kPrismOk) return false;
        if (strcmp(r.voice, name) != 0) {
            snprintf(r.voice, sizeof(r.voice), "%s", name);
            acclog::Write("Speech", "render[%s] voice [%s] (id %zu)", r.tag, name, i);
        }
        return true;
    }
    acclog::Write("Speech", "render[%s]: none of %zu voices matches [%s]", r.tag, count, wanted);
    return false;
}

// One render at full volume on a render-only backend. `voice` null = the
// backend's default; `ratePercent` -1 = the voice's default rate (Prism 0.5).
bool RenderOn(RenderBackend& r, const char* voice, int ratePercent, const char* text,
              RenderedSpeech& out) {
    out = RenderedSpeech{};
    if (!text || !*text) return false;
    if (!EnsureRenderBackend(r)) return false;
    if (voice && !SelectVoice(r, voice)) return false;
    pPrism_backend_set_rate(r.be, ratePercent >= 0 ? ClampPercent(ratePercent) / 100.0f : 0.5f);
    pPrism_backend_set_volume(r.be, 1.0f);

    const std::string utf8 = ToUtf8(text);
    if (utf8.empty()) return false;
    const DWORD start = GetTickCount();
    DWORD seh = 0;
    PrismError rc = SehSpeakToMemory(r.be, utf8.c_str(), CollectRenderedSpeech, &out, &seh);
    const DWORD elapsed = GetTickCount() - start;
    if (seh != 0) {
        acclog::Write("Speech", "render[%s] faulted (%s 0x%08lX)", r.tag, SehCodeName(seh), seh);
        return false;
    }
    if (rc != kPrismOk || out.samples.empty() || out.channels == 0 || out.sampleRate == 0) {
        acclog::Write("Speech", "render[%s] failed rc=%d samples=%zu ch=%zu rate=%zu (%lu ms)",
                      r.tag, rc, out.samples.size(), out.channels, out.sampleRate, elapsed);
        return false;
    }
    acclog::Write("Speech", "render[%s] ok: %zu samples, %zu ch, %zu Hz, %.2f s audio in %lu ms [%s]",
                  r.tag, out.samples.size(), out.channels, out.sampleRate,
                  static_cast<double>(out.samples.size() / out.channels) / out.sampleRate,
                  elapsed, text);
    return true;
}

}  // namespace

bool RenderUrgentToMemory(const char* text, int ratePercent, RenderedSpeech& out) {
    // The urgent channel's voice on the render-only instance, at full volume:
    // the urgent-volume slider is tuned for centred speech at system level,
    // not for a line inside the game's mix — positional speech sets its own.
    char voice[kVoiceNameMax] = {};
    const bool haveVoice = g_sapiVoiceCurrent != kNoVoice &&
                           FetchSapiVoiceNameUtf8(g_sapiVoiceCurrent, voice, sizeof(voice));
    return RenderOn(g_renderSapi, haveVoice ? voice : nullptr,
                    ratePercent >= 0 ? ratePercent : LoadPercent(g_sapiRate), text, out);
}

bool RenderSapiVoiceToMemory(const char* voiceName, int ratePercent, const char* text,
                             RenderedSpeech& out) {
    if (!voiceName) { out = RenderedSpeech{}; return false; }
    return RenderOn(g_renderSapi, voiceName, ratePercent, text, out);
}

bool RenderOneCoreToMemory(const char* voiceName, int ratePercent, const char* text,
                           RenderedSpeech& out) {
    if (!voiceName) { out = RenderedSpeech{}; return false; }
    return RenderOn(g_renderOneCore, voiceName, ratePercent, text, out);
}

}  // namespace prism
