# prism_internal.h (231 lines)

Source: `patches/Accessibility/prism_internal.h`

Internal to the prism module (prism*.cpp); no other file includes it. The public interface is prism.h. Here: what the files share.

## Declarations (in source order)

- L24 `typedef struct PrismContext PrismContext`: Subset of Prism's C ABI we use.
- L25 `typedef struct PrismBackend PrismBackend`
- L26 `typedef uint64_t PrismBackendId`
- L27 `typedef int PrismError`
- L63 `typedef PrismContext* (__cdecl* PFN_prism_init)(void*)`: prism_init takes a void* and is always handed nullptr.
- L64 `typedef void (__cdecl* PFN_prism_shutdown)(PrismContext*)`
- L65 `typedef bool (__cdecl* PFN_prism_registry_exists)(PrismContext*, PrismBackendId)`
- L66 `typedef size_t (__cdecl* PFN_prism_registry_count)(PrismContext*)`
- L67 `typedef PrismBackendId (__cdecl* PFN_prism_registry_id_at)(PrismContext*, size_t)`
- L68 `typedef PrismBackend* (__cdecl* PFN_prism_registry_acquire)(PrismContext*, PrismBackendId)`
- L69 `typedef PrismBackend* (__cdecl* PFN_prism_registry_acquire_best)(PrismContext*)`
- L71 `typedef PrismBackend* (__cdecl* PFN_prism_registry_create)(PrismContext*, PrismBackendId)`: A FRESH backend instance, never the cached one acquire hands out.
- L72 `typedef uint64_t (__cdecl* PFN_prism_backend_get_features)(PrismBackend*)`
- L73 `typedef PrismError (__cdecl* PFN_prism_backend_initialize)(PrismBackend*)`
- L74 `typedef PrismError (__cdecl* PFN_prism_backend_speak)(PrismBackend*, const char*, bool)`
- L75 `typedef PrismError (__cdecl* PFN_prism_backend_output)(PrismBackend*, const char*, bool)`
- L76 `typedef PrismError (__cdecl* PFN_prism_backend_braille)(PrismBackend*, const char*)`
- L77 `typedef PrismError (__cdecl* PFN_prism_backend_stop)(PrismBackend*)`
- L78 `typedef PrismError (__cdecl* PFN_prism_backend_is_speaking)(PrismBackend*, bool*)`
- L79 `typedef PrismError (__cdecl* PFN_prism_backend_set_rate)(PrismBackend*, float)`
- L80 `typedef PrismError (__cdecl* PFN_prism_backend_set_volume)(PrismBackend*, float)`
- L81 `typedef PrismError (__cdecl* PFN_prism_backend_set_voice)(PrismBackend*, size_t)`
- L82 `typedef PrismError (__cdecl* PFN_prism_backend_count_voices)(PrismBackend*, size_t*)`
- L83 `typedef PrismError (__cdecl* PFN_prism_backend_get_voice_name)(PrismBackend*, size_t, const char**)`
- L84 `typedef PrismError (__cdecl* PFN_prism_backend_get_voice)(PrismBackend*, size_t*)`
- L85 `typedef const char* (__cdecl* PFN_prism_backend_name)(PrismBackend*)`
- L88 `typedef void (__cdecl* PFN_PrismAudioCallback)(void* userdata, const float* samples, size_t sample_count, size_t channels, size_t sample_rate)`: Render-to-PCM.
- L91 `typedef PrismError (__cdecl* PFN_prism_backend_speak_to_memory)(PrismBackend*, const char*, PFN_PrismAudioCallback, void*)`
- L189 `struct RenderBackend`: Render-only backends: separate instances that never speak aloud.
- L202 `const char* SehCodeName(DWORD code)`: prism.cpp
- L203 `PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`
- L204 `PrismBackend* SehCreate(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`
- L205 `PrismError SehInitialize(PrismBackend* be, DWORD* outCode)`
- L206 `void BrailleNormal(const char* utf8)`
- L207 `bool AllViaScreenReader()`
- L208 `void SpeakUrgentImpl(const char* text, bool interrupt)`
- L211 `bool ReencodeAcpToUtf8(const char* in, char* outBuf, size_t outBufSize)`: prism_text.cpp
- L212 `bool ReencodeUtf8ToGameCodepage(const char* in, char* outBuf, size_t outBufSize)`
- L213 `void CaptureSpoken(const char* text)`
- L214 `void CaptureSpokenW(const wchar_t* text)`
- L217 `void EnsureSapiRateLoaded()`: prism_urgent.cpp
- L218 `bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize)`
- L219 `size_t FindSapiVoiceByNameUtf8(const char* utf8Name)`
- L220 `bool TryAcquireSapi()`
- L223 `bool TryCreateCompass()`: prism_compass.cpp
- L224 `void DropCompassFollowUp(const char* why)`
- L227 `float RateFromPercent(int percent)`: prism_render.cpp
- L228 `bool RenderOn(RenderBackend& r, const char* voice, float rate, float volume, const char* text, RenderedSpeech& out)`

Also 55 constants or variables at namespace scope.
