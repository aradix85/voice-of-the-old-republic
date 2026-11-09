// Internal to the prism module (prism*.cpp); no other file includes
// it. The public interface is prism.h. Here: what the files share.

#pragma once

#include "prism.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include "bridge.h"   // spoken-line ring buffer for the local bridge
#include "log.h"
#include "mod_settings_store.h"  // persist the SAPI urgent-volume slider
#include "menus_modsettings.h"   // "Speak everything with the screen reader"

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
constexpr PrismError kPrismErrBackendNotAvailable = 16;
constexpr PrismError kPrismErrUnknown             = 17;  // sentinel for SEH faults
constexpr uint64_t  kPrismBackendIdSapi           = 0x1D6DF72422CEEE66ull;
// FNV-1a of "OneCore", Prism's backend_catalog.h (the same hash yields the
// SAPI id above).
constexpr uint64_t  kPrismBackendIdOneCore        = 0x6797D32F0D994CB4ull;

// Default SAPI speech rate for the urgent channel, in the percent the
// Mod-settings slider speaks. Prism maps [0.0..1.0] to SAPI's -10..+10 with
// 0.5 = SAPI default (midpoint-linear); 80% lands at roughly SAPI +6 —
// distinctly faster than out-of-the-box SAPI so urgent map-cursor /
// region-cursor / walking cues don't drag during continuous WASD panning,
// still well within intelligibility for the bundled Microsoft voices. The
// user can move it either way from the announcement-speed slider.
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

extern HMODULE       g_prismLib;
extern PrismContext* g_prismCtx;
extern PrismBackend* g_prismNormal;  // NVDA / JAWS / OneCore / ... — chosen by acquire_best
extern PrismBackend* g_prismSapi;  // urgent channel; SAPI bypass
extern bool          g_initTried;
extern bool          g_normalReady;
extern bool          g_sapiReady;

// Tracks the SAPI voice id last applied via prism_backend_set_voice. SIZE_MAX
// = "unknown / not yet set"; first call will always issue set_voice so the
// backend's state matches our cache. Voice swap costs nothing in steady state
// — we only call set_voice when the requested id differs.
extern size_t g_sapiVoiceCurrent;
// Cached voice count so out-of-range requests fall back silently instead of
// asking SAPI for a voice it doesn't have. 0 = not enumerated / no voices.
extern size_t g_sapiVoiceCount;

// The urgent channel's voice, persisted BY NAME rather than by index: SAPI
// numbers voices in enumeration order, so installing or removing any voice
// renumbers the rest and a saved index would land the user on a different
// voice — quite possibly one speaking a different language. The name is
// stored in Prism's own encoding (UTF-8), which is what the id lookup
// compares against. Empty = never chosen; the channel keeps SAPI's default.
constexpr const char* kSapiVoiceKey = "UrgentVoiceName";
extern char g_sapiVoiceName[160];
extern bool g_sapiVoiceLoaded;

// Same shape for the urgent-channel speech rate — persisted percent [0,100],
// pushed to the backend through ApplySapiRate. The default is
// kSapiRateDefaultPercent, the rate the channel ran at before the slider
// existed, so an install that never touches the row sounds unchanged.
constexpr const char* kSapiRateKey = "UrgentRatePercent";
extern int  g_sapiRatePercent;
extern bool g_sapiRateLoaded;

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

// ---- Last-utterance capture ---------------------------------------------
//
// Held in the game codepage — the same bytes the caller handed us — so
// consumers can re-widen with the codepage of their choice. Sized well above
// kAnnounceTextMax (1024) so we never truncate an utterance the speech path
// itself carried in full; over-long text is truncated rather than dropped.
extern char s_lastSpoken[4096];
extern bool s_suppressNextCapture;

// ---- Compass channel ------------------------------------------------------
//
// A second speaking SAPI instance, so the compass readout while turning has a
// voice of its own: the player tells a heading from an announcement by the
// voice alone, and neither cuts the other off — each instance owns its own
// SpVoice and audio stream. prism_registry_create builds it fresh;
// prism_registry_acquire would hand back the urgent channel's cached one. Both
// instances enumerate the same SAPI tokens, so voice ids and names are shared
// with the urgent picker (g_sapiVoiceCount, FetchSapiVoiceNameUtf8).
//
// Volume, rate and voice persist like the urgent channel's, the voice by name.
// Without a saved voice the channel takes kCompassDefaultVoiceId, or the first
// voice that is not the urgent one when that id is missing or is the urgent
// voice itself: the default only has to sound different from the
// announcements. It is re-derived each launch until the player chooses one.
extern PrismBackend* g_prismCompass;
extern bool          g_compassReady;
extern size_t        g_compassVoiceCurrent;
constexpr size_t      kCompassDefaultVoiceId = 2;
constexpr const char* kCompassVoiceKey       = "CompassVoiceName";

// Render-only backends: separate instances that never speak aloud. A render on
// the urgent channel's own SAPI voice would queue behind (and block the game
// on) whatever that voice is still saying — SAPI's speak-to-memory is a
// synchronous Speak on the same SpVoice. prism_registry_create builds a fresh
// instance; prism_registry_acquire would hand back the cached urgent one.
// Created on first use, on the game thread, and only ever used there.
struct RenderBackend {
    const char*   tag;
    uint64_t      id;
    PrismBackend* be;
    bool          tried;
    char          voice[160];   // last voice selected, to log only changes
};
extern RenderBackend g_renderSapi;
extern RenderBackend g_renderOneCore;

// Helpers shared across the files, by the file that defines them.

// prism.cpp
const char* SehCodeName(DWORD code);
PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode);
PrismBackend* SehCreate(PrismContext* ctx, PrismBackendId id, DWORD* outCode);
PrismError SehInitialize(PrismBackend* be, DWORD* outCode);
void BrailleNormal(const char* utf8);
bool AllViaScreenReader();
void SpeakUrgentImpl(const char* text, bool interrupt);

// prism_text.cpp
bool ReencodeAcpToUtf8(const char* in, char* outBuf, size_t outBufSize);
bool ReencodeUtf8ToGameCodepage(const char* in, char* outBuf, size_t outBufSize);
void CaptureSpoken(const char* text);
void CaptureSpokenW(const wchar_t* text);

// prism_urgent.cpp
void EnsureSapiRateLoaded();
bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize);
size_t FindSapiVoiceByNameUtf8(const char* utf8Name);
bool TryAcquireSapi();

// prism_compass.cpp
bool TryCreateCompass();
void DropCompassFollowUp(const char* why);

// prism_render.cpp
float RateFromPercent(int percent);
bool RenderOn(RenderBackend& r, const char* voice, float rate, float volume, const char* text, RenderedSpeech& out);

}  // namespace prism
