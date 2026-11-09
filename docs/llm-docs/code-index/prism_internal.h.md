# prism_internal.h (175 lines)

Source: `patches/Accessibility/prism_internal.h`

Internal to the prism module (prism*.cpp); no other file includes it. The public interface is prism.h. Here: what the files share.

## Declarations (in source order)

- L18 `typedef struct PrismContext PrismContext`: Subset of Prism's C ABI we use.
- L19 `typedef struct PrismBackend PrismBackend`
- L20 `typedef uint64_t PrismBackendId`
- L21 `typedef int PrismError`
- L34 `inline bool InitOk(PrismError rc)`: A backend that reports ALREADY_INITIALIZED is a cached, working instance.
- L58 `typedef PrismContext* (__cdecl* PFN_prism_init)(void*)`: prism_init takes a void* and is always handed nullptr.
- L59 `typedef void (__cdecl* PFN_prism_shutdown)(PrismContext*)`
- L60 `typedef bool (__cdecl* PFN_prism_registry_exists)(PrismContext*, PrismBackendId)`
- L61 `typedef size_t (__cdecl* PFN_prism_registry_count)(PrismContext*)`
- L62 `typedef PrismBackendId (__cdecl* PFN_prism_registry_id_at)(PrismContext*, size_t)`
- L63 `typedef PrismBackend* (__cdecl* PFN_prism_registry_acquire)(PrismContext*, PrismBackendId)`
- L64 `typedef PrismBackend* (__cdecl* PFN_prism_registry_acquire_best)(PrismContext*)`
- L66 `typedef PrismBackend* (__cdecl* PFN_prism_registry_create)(PrismContext*, PrismBackendId)`: A FRESH backend instance, never the cached one acquire hands out.
- L67 `typedef uint64_t (__cdecl* PFN_prism_backend_get_features)(PrismBackend*)`
- L68 `typedef PrismError (__cdecl* PFN_prism_backend_initialize)(PrismBackend*)`
- L69 `typedef PrismError (__cdecl* PFN_prism_backend_speak)(PrismBackend*, const char*, bool)`
- L70 `typedef PrismError (__cdecl* PFN_prism_backend_output)(PrismBackend*, const char*, bool)`
- L71 `typedef PrismError (__cdecl* PFN_prism_backend_braille)(PrismBackend*, const char*)`
- L72 `typedef PrismError (__cdecl* PFN_prism_backend_stop)(PrismBackend*)`
- L73 `typedef PrismError (__cdecl* PFN_prism_backend_is_speaking)(PrismBackend*, bool*)`
- L74 `typedef PrismError (__cdecl* PFN_prism_backend_set_rate)(PrismBackend*, float)`
- L75 `typedef PrismError (__cdecl* PFN_prism_backend_set_volume)(PrismBackend*, float)`
- L76 `typedef PrismError (__cdecl* PFN_prism_backend_set_voice)(PrismBackend*, size_t)`
- L77 `typedef PrismError (__cdecl* PFN_prism_backend_count_voices)(PrismBackend*, size_t*)`
- L78 `typedef PrismError (__cdecl* PFN_prism_backend_get_voice_name)(PrismBackend*, size_t, const char**)`
- L79 `typedef PrismError (__cdecl* PFN_prism_backend_get_voice)(PrismBackend*, size_t*)`
- L80 `typedef const char* (__cdecl* PFN_prism_backend_name)(PrismBackend*)`
- L83 `typedef void (__cdecl* PFN_PrismAudioCallback)(void* userdata, const float* samples, size_t sample_count, size_t channels, size_t sample_rate)`: Render-to-PCM.
- L86 `typedef PrismError (__cdecl* PFN_prism_backend_speak_to_memory)(PrismBackend*, const char*, PFN_PrismAudioCallback, void*)`
- L134 `struct PercentSetting`: A persisted percent [0,100], pulled from the store on first access.
- L139 `inline int ClampPercent(int v)`
- L145 `const char* SehCodeName(DWORD code)`: prism.cpp
- L146 `PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`
- L147 `PrismError SehInitialize(PrismBackend* be, DWORD* outCode)`
- L148 `PrismBackend* CreateBackend(PrismBackendId id, PrismError* rc, DWORD* seh)`
- L149 `void BrailleNormal(const char* utf8)`
- L150 `bool AllViaScreenReader()`
- L151 `void SpeakUrgentImpl(const char* text, bool interrupt)`
- L154 `std::string ToUtf8(const char* gameText)`: prism_text.cpp
- L155 `std::string FromUtf8(const char* utf8)`
- L156 `std::string WideToUtf8(const wchar_t* text)`
- L157 `void CaptureSpoken(const char* text)`
- L158 `void CaptureSpokenW(const wchar_t* text)`
- L161 `int LoadPercent(PercentSetting& s)`: prism_urgent.cpp
- L162 `void StorePercent(PercentSetting& s, int v)`
- L163 `void ApplyLevels(PrismBackend* be, PercentSetting& volume, PercentSetting& rate, const char* channel)`
- L165 `bool SetVoiceCached(PrismBackend* be, size_t& current, size_t voiceId, const char* channel)`
- L166 `bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize)`
- L167 `size_t FindSapiVoiceByNameUtf8(const char* utf8Name)`
- L168 `bool TryAcquireSapi()`
- L171 `bool TryCreateCompass()`: prism_compass.cpp
- L172 `void DropCompassFollowUp(const char* why)`

Also 41 constants or variables at namespace scope.
