// Internal to the prism module (prism*.cpp); no other file includes
// it. The public interface is prism.h. Here: what the files share.

#pragma once

#include "prism.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace prism {

// Subset of Prism's C ABI we use. Matches third_party/prism-dist/include/prism.h
// verbatim. We declare the function-pointer types here rather than including
// prism.h so the patch DLL build has no header dependency on the upstream tree.
typedef struct PrismContext PrismContext;
typedef struct PrismBackend PrismBackend;
typedef uint64_t PrismBackendId;
typedef int PrismError;  // 0 == PRISM_OK

constexpr PrismError kPrismOk                     = 0;
// The backend has no implementation of the entry point — e.g. braille on a
// speech-only backend, or ZDSR running against a ZDSRAPI that predates its
// Braille export.
constexpr PrismError kPrismErrNotImplemented      = 3;
constexpr PrismError kPrismErrAlreadyInitialized  = 15;  // see PrismError enum
constexpr PrismError kPrismErrUnknown             = 17;  // sentinel for SEH faults
// FNV-1a of "SAPI", Prism's backend_catalog.h.
constexpr uint64_t  kPrismBackendIdSapi           = 0x1D6DF72422CEEE66ull;

// A backend that reports ALREADY_INITIALIZED is a cached, working instance.
inline bool InitOk(PrismError rc) {
    return rc == kPrismOk || rc == kPrismErrAlreadyInitialized;
}

// Default SAPI speech rate for the urgent and compass channels, in the percent
// the Mod-settings slider speaks. Prism maps [0.0..1.0] to SAPI's -10..+10 with
// 0.5 = SAPI default (midpoint-linear); 80% lands at roughly SAPI +6 —
// distinctly faster than out-of-the-box SAPI so urgent map-cursor /
// region-cursor / walking cues don't drag during continuous WASD panning,
// still well within intelligibility for the bundled Microsoft voices.
constexpr int kSapiRateDefaultPercent = 80;

// prism_init takes a void* and is always handed nullptr. It reads nothing from
// a NULL config and falls back to the global backend registry, which is exactly
// what we want — and that avoids declaring PrismConfig at all. That struct is
// version-specific in a way no single declaration survives: in 0.16.5 it was a
// lone version byte and prism_init required version == PRISM_CONFIG_VERSION
// exactly; in 0.17.3 it carries a registry pointer, an availability callback
// plus userdata, three uint32 tuning fields and a bool, and the check relaxed
// to ">". A config built for either version is rejected outright by the other.
// prism_config_init is deliberately not resolved for the same reason: it
// returns PrismConfig by value, so its calling convention changes shape with
// the struct — under 0.17.3 it returns through a hidden pointer this code
// would never pass, which corrupts the stack rather than failing cleanly.
typedef PrismContext*  (__cdecl* PFN_prism_init)(void*);
typedef void           (__cdecl* PFN_prism_shutdown)(PrismContext*);
typedef bool           (__cdecl* PFN_prism_registry_exists)(PrismContext*, PrismBackendId);
typedef size_t         (__cdecl* PFN_prism_registry_count)(PrismContext*);
typedef PrismBackendId (__cdecl* PFN_prism_registry_id_at)(PrismContext*, size_t);
typedef PrismBackend*  (__cdecl* PFN_prism_registry_acquire)(PrismContext*, PrismBackendId);
typedef PrismBackend*  (__cdecl* PFN_prism_registry_acquire_best)(PrismContext*);
// A FRESH backend instance, never the cached one acquire hands out.
typedef PrismBackend*  (__cdecl* PFN_prism_registry_create)(PrismContext*, PrismBackendId);
typedef uint64_t       (__cdecl* PFN_prism_backend_get_features)(PrismBackend*);
typedef PrismError     (__cdecl* PFN_prism_backend_initialize)(PrismBackend*);
typedef PrismError     (__cdecl* PFN_prism_backend_speak)(PrismBackend*, const char*, bool);
typedef PrismError     (__cdecl* PFN_prism_backend_output)(PrismBackend*, const char*, bool);
typedef PrismError     (__cdecl* PFN_prism_backend_braille)(PrismBackend*, const char*);
typedef PrismError     (__cdecl* PFN_prism_backend_stop)(PrismBackend*);
typedef PrismError     (__cdecl* PFN_prism_backend_is_speaking)(PrismBackend*, bool*);
typedef PrismError     (__cdecl* PFN_prism_backend_set_rate)(PrismBackend*, float);
typedef PrismError     (__cdecl* PFN_prism_backend_set_volume)(PrismBackend*, float);
typedef PrismError     (__cdecl* PFN_prism_backend_set_voice)(PrismBackend*, size_t);
typedef PrismError     (__cdecl* PFN_prism_backend_count_voices)(PrismBackend*, size_t*);
typedef PrismError     (__cdecl* PFN_prism_backend_get_voice_name)(PrismBackend*, size_t, const char**);
typedef PrismError     (__cdecl* PFN_prism_backend_get_voice)(PrismBackend*, size_t*);
typedef const char*    (__cdecl* PFN_prism_backend_name)(PrismBackend*);
// Render-to-PCM. The callback receives interleaved float samples; sample_count
// counts every channel's samples (frames × channels), per the SAPI backend.
typedef void           (__cdecl* PFN_PrismAudioCallback)(void* userdata, const float* samples,
                                                         size_t sample_count, size_t channels,
                                                         size_t sample_rate);
typedef PrismError     (__cdecl* PFN_prism_backend_speak_to_memory)(PrismBackend*, const char*,
                                                                   PFN_PrismAudioCallback, void*);

// Init resolves every pointer before it sets g_prismCtx, and fails without
// one (the bundled prism.dll exports them all). Code that runs only with a
// context or a backend in hand calls them unchecked.
extern PFN_prism_init                  pPrism_init;
extern PFN_prism_shutdown              pPrism_shutdown;
extern PFN_prism_registry_exists       pPrism_registry_exists;
extern PFN_prism_registry_count        pPrism_registry_count;
extern PFN_prism_registry_id_at        pPrism_registry_id_at;
extern PFN_prism_registry_acquire      pPrism_registry_acquire;
extern PFN_prism_registry_acquire_best pPrism_registry_acquire_best;
extern PFN_prism_registry_create       pPrism_registry_create;
extern PFN_prism_backend_get_features  pPrism_backend_get_features;
extern PFN_prism_backend_initialize    pPrism_backend_initialize;
extern PFN_prism_backend_speak         pPrism_backend_speak;
extern PFN_prism_backend_output        pPrism_backend_output;
extern PFN_prism_backend_braille       pPrism_backend_braille;
extern PFN_prism_backend_stop          pPrism_backend_stop;
extern PFN_prism_backend_is_speaking   pPrism_backend_is_speaking;
extern PFN_prism_backend_set_rate      pPrism_backend_set_rate;
extern PFN_prism_backend_set_volume    pPrism_backend_set_volume;
extern PFN_prism_backend_set_voice     pPrism_backend_set_voice;
extern PFN_prism_backend_count_voices  pPrism_backend_count_voices;
extern PFN_prism_backend_get_voice_name pPrism_backend_get_voice_name;
extern PFN_prism_backend_get_voice     pPrism_backend_get_voice;
extern PFN_prism_backend_name          pPrism_backend_name;
extern PFN_prism_backend_speak_to_memory pPrism_backend_speak_to_memory;

extern PrismContext* g_prismCtx;
extern PrismBackend* g_prismNormal;    // NVDA / JAWS / OneCore / ... — chosen by acquire_best
extern PrismBackend* g_prismSapi;      // urgent channel; SAPI bypass
extern PrismBackend* g_prismCompass;   // compass channel; a second SAPI instance
extern bool          g_normalReady;
extern bool          g_sapiReady;
extern bool          g_compassReady;

// SAPI voices, shared by the urgent and compass channels (both instances
// enumerate the same tokens, so ids and names agree). g_sapiVoiceCurrent is
// the urgent channel's voice, kNoVoice until known; g_sapiVoiceCount is 0
// until enumerated or when SAPI has none.
constexpr size_t kNoVoice      = static_cast<size_t>(-1);
constexpr size_t kVoiceNameMax = 160;   // UTF-8 bytes, NUL included
extern size_t g_sapiVoiceCurrent;
extern size_t g_sapiVoiceCount;

// A persisted percent [0,100], pulled from the store on first access.
struct PercentSetting {
    const char* key;
    int         value;   // the default until loaded
    bool        loaded;
};
inline int ClampPercent(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
extern PercentSetting g_sapiRate;   // the urgent channel's rate

// Helpers shared across the files, by the file that defines them.

// prism.cpp
const char* SehCodeName(DWORD code);
PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode);
PrismError SehInitialize(PrismBackend* be, DWORD* outCode);
PrismBackend* CreateBackend(PrismBackendId id, PrismError* rc, DWORD* seh);
void BrailleNormal(const char* utf8);
bool AllViaScreenReader();
void SpeakUrgentImpl(const char* text, bool interrupt);

// prism_text.cpp
std::string ToUtf8(const char* gameText);
std::string FromUtf8(const char* utf8);
std::string WideToUtf8(const wchar_t* text);
void CaptureSpoken(const char* text);
void CaptureSpokenW(const wchar_t* text);

// prism_urgent.cpp
int LoadPercent(PercentSetting& s);
void StorePercent(PercentSetting& s, int v);
void ApplyLevels(PrismBackend* be, PercentSetting& volume, PercentSetting& rate,
                 const char* channel);
bool SetVoiceCached(PrismBackend* be, size_t& current, size_t voiceId, const char* channel);
bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize);
size_t FindSapiVoiceByNameUtf8(const char* utf8Name);
bool TryAcquireSapi();

// prism_compass.cpp
bool TryCreateCompass();
void DropCompassFollowUp(const char* why);

}  // namespace prism
