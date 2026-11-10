#include "audio_bus.h"
#include "audio_cues.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

#include "audio_pitch.h"   // BeginScopedZero / EndScopedZero (per-call jitter)
#include "engine_player.h"
#include "log.h"
#include "mod_settings_store.h"  // persist the cue-volume slider across launches
#include "engine_rebase.h"
#include "engine_offsets_select.h"
#include "engine_game.h"

namespace acc::audio {

// Declared in audio_bus.h so audio_loop.cpp shares it rather than mirroring.
void FillResRef(CResRef& out, const char* tag) {
    std::memset(out.string, 0, sizeof(out.string));
    if (!tag) return;
    size_t n = std::strlen(tag);
    if (n > sizeof(out.string)) n = sizeof(out.string);
    for (size_t i = 0; i < n; ++i) {
        char c = tag[i];
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
        out.string[i] = c;
    }
}

namespace {

// CExoSound::PlayOneShotSound — __thiscall. Verified by decompile of the
// inner CExoSoundInternal::PlayOneShotSound. Earlier typedef mislabelled
// param_4 as "looping" and param_5/6 as "volume"/"pan"; the real layout is:
//   priority_group, delay_ms, volume_byte, fixed_variance, pitch_variance.
typedef void (__thiscall* PFN_PlayOneShotSound)(
    void*    this_,
    const CResRef* res,
    uint8_t  priority_group,
    uint32_t delay_ms,
    uint8_t  volume_byte,     // 0 = group default (127)
    float    fixed_variance,
    float    pitch_variance);

// CExoSound::Play3DOneShotSound — __thiscall. Same fields as the 2D
// variant plus position (pass-by-value, 12 bytes) and z_offset.
typedef void (__thiscall* PFN_Play3DOneShotSound)(
    void*    this_,
    const CResRef* res,
    Vector   position,
    float    z_offset,
    uint8_t  priority_group,
    uint32_t delay_ms,
    uint8_t  volume_byte,
    float    fixed_variance,
    float    pitch_variance);

void* GetCExoSound() {
    __try {
        return *reinterpret_cast<void**>(kAddrCExoSoundPtr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// --- Global cue volume ----------------------------------------------
// Percent [0,100]; 100 = each cue at its base volume. Backed by the
// persistent store (acc_settings.ini) so it survives relaunch — lazily
// pulled on first use so a cue or the menu sees the saved value even if
// the other never ran this session.
constexpr const char* kCueVolumeKey = "CueVolumePercent";
int  g_cueVolumePercent = 100;
bool g_cueVolumeLoaded  = false;

void EnsureCueVolumeLoaded() {
    if (g_cueVolumeLoaded) return;
    g_cueVolumeLoaded = true;
    int v = acc::settings::GetInt(kCueVolumeKey, 100);
    if (v < 0)   v = 0;
    if (v > 100) v = 100;
    g_cueVolumePercent = v;
}

// baseVolume × slider, rounded, clamped to [0,127]. Returns 0 to mean
// "muted — skip the engine call" (volume_byte==0 means engine-full, not
// silence, so the callers must not forward a 0).
uint8_t EffectiveVolumeByte(uint8_t baseVolume) {
    EnsureCueVolumeLoaded();
    int v = (static_cast<int>(baseVolume) * g_cueVolumePercent + 50) / 100;
    if (v > 127) v = 127;
    if (v < 0)   v = 0;
    return static_cast<uint8_t>(v);
}

// --- Priority-group resolution (live CPriorityGroup table) ----------
// Layout established by the priority-groups probe against the XML type DB
// (2026-05-14). That probe was retired once its findings landed here; see
// probe_priority_groups.cpp in git history if the derivation is needed:
//   CExoSound facade   +0x00 -> CExoSoundInternal*
//   CExoSoundInternal  +0x4c -> CPriorityGroup* (heap array)
//   CPriorityGroup stride 0x18: +0x06 priority(byte) +0x07 volume(byte)
//                               +0x14 fade_time(ushort)
// K2 witnesses: every K2 CExoSound facade reads the internal at [facade+0]
// (kSoundInternalOffset Same); GetPriorityGroup's twin 0x00709010 indexes
// `[exoInternal+0x50] + group*0x18` (table ptr moved 0x4c→0x50, stride
// unchanged), and SetPriorityGroup's internal twin 0x007101F0 reads the
// group's volume byte at +7 — the same slot pair as KOTOR 1. FadeTime at
// +0x14 rides the unchanged stride/layout (the sentinel-row scan tolerates
// a miss by falling back to group 26).
const size_t   kSoundInternalOffset      = acc::off::Same(0x00);
const size_t   kPriorityGroupsPtrOff     = acc::off::Pick(0x4c, 0x50);
const size_t   kPriorityGroupStride      = acc::off::Same(0x18);
const size_t   kPriorityGroupPriorityOff = acc::off::Same(0x06);
const size_t   kPriorityGroupVolumeOff   = acc::off::Same(0x07);
const size_t   kPriorityGroupFadeTimeOff = acc::off::Same(0x14);

// Our installer stamps this value into the FadeTime column of the custom
// full-volume row it appends to prioritygroups.2da. Vanilla fade times
// are 0 or 1000, so 31337 is an unmistakable fingerprint — and because we
// match on a struct field the engine actually loads (not the row index),
// we find our group wherever it lands, immune to row-index drift from
// other mods that also extend the table.
constexpr uint16_t kCueGroupSentinelFadeTime = 31337;

// Fallback when the sentinel row isn't present (prioritygroups.2da edit
// not yet applied): vanilla group 26 — volume 127, identical 20m/10m
// falloff to the legacy group 0, not pause-exempt. Index 26 is within the
// 27-row vanilla table.
constexpr uint8_t  kFallbackFullGroup        = 26;

constexpr int      kMaxGroupScan             = 40;

}  // namespace

void* GetSoundInternal() {
    void* exoSound = GetCExoSound();
    if (!exoSound) return nullptr;
    __try {
        return *reinterpret_cast<void**>(
            reinterpret_cast<char*>(exoSound) + kSoundInternalOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void SetGlobalCueVolumePercent(int percent) {
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    g_cueVolumeLoaded  = true;  // we are the authoritative value now
    g_cueVolumePercent = percent;
    acc::settings::SetInt(kCueVolumeKey, percent);  // persist across launches
    acclog::Write("AudioBus", "cue volume set to %d%%", percent);
}

int GetGlobalCueVolumePercent() {
    EnsureCueVolumeLoaded();
    return g_cueVolumePercent;
}

namespace {

// Scan the live CPriorityGroup table for a row whose FadeTime equals `sentinel`.
// Returns: the group index (>=0) if found; kScanNotReady if the sound subsystem
// isn't initialised yet (caller should retry, NOT cache a fallback); or
// kScanAbsent if the table was read but no row carries the sentinel.
constexpr int kScanNotReady = -1;
constexpr int kScanAbsent   = -2;

int ScanForSentinelGroup(uint16_t sentinel) {
    void* exoSound = GetCExoSound();
    if (!exoSound) return kScanNotReady;

    __try {
        void* internal = *reinterpret_cast<void**>(
            reinterpret_cast<char*>(exoSound) + kSoundInternalOffset);
        if (!internal) return kScanNotReady;
        void* table = *reinterpret_cast<void**>(
            reinterpret_cast<char*>(internal) + kPriorityGroupsPtrOff);
        if (!table) return kScanNotReady;

        int garbageRun = 0;
        for (int i = 0; i < kMaxGroupScan; ++i) {
            auto* e = reinterpret_cast<unsigned char*>(table) +
                      static_cast<size_t>(i) * kPriorityGroupStride;
            uint8_t vol  = *(e + kPriorityGroupVolumeOff);
            uint8_t prio = *(e + kPriorityGroupPriorityOff);
            // Same off-the-end heuristic the probe uses: bail after a run
            // of clearly-invalid rows (both bytes > 127).
            if (vol > 127 && prio > 127) {
                if (++garbageRun >= 4) break;
                continue;
            }
            garbageRun = 0;
            uint16_t fade = *reinterpret_cast<uint16_t*>(
                e + kPriorityGroupFadeTimeOff);
            if (fade == sentinel) return i;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return kScanNotReady;
    }
    return kScanAbsent;
}

}  // namespace

uint8_t GetCuePriorityGroup() {
    // -1 = not yet resolved. Cached after a successful scan (the table is
    // built once at sound init and never moves). A not-ready subsystem returns
    // the fallback WITHOUT caching, so a cue that fires before sound init
    // doesn't pin us to the fallback forever.
    static int s_resolved = -1;
    if (s_resolved >= 0) return static_cast<uint8_t>(s_resolved);

    int r = ScanForSentinelGroup(kCueGroupSentinelFadeTime);
    if (r >= 0) {
        s_resolved = r;
        acclog::Write("AudioBus",
            "cue group resolved to dedicated sentinel group %d", r);
        return static_cast<uint8_t>(r);
    }
    if (r == kScanNotReady) return kFallbackFullGroup;  // retry next cue

    // Sentinel absent — cache the fallback so we don't rescan per cue.
    s_resolved = kFallbackFullGroup;
    acclog::Write("AudioBus",
        "cue group sentinel not found; using fallback full group %u",
        (unsigned)kFallbackFullGroup);
    return kFallbackFullGroup;
}

bool PlayCue(const char* resref, uint8_t priorityGroup,
             uint8_t baseVolume) {
    if (!resref || !*resref) return false;
    void* exoSound = GetCExoSound();
    if (!exoSound) return false;

    uint8_t vol = EffectiveVolumeByte(baseVolume);
    if (vol == 0) return false;  // muted — skip (0 would be engine-full)
    uint8_t group = priorityGroup ? priorityGroup : GetCuePriorityGroup();

    CResRef res;
    FillResRef(res, resref);

    pitch::BeginScopedZero();
    __try {
        auto fn = reinterpret_cast<PFN_PlayOneShotSound>(
            kAddrCExoSoundPlayOneShotSound);
        fn(exoSound, &res,
           group,
           /*delay_ms=*/0,
           vol,
           /*fixed_variance=*/0.0f,
           /*pitch_variance=*/0.0f);
        pitch::EndScopedZero();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pitch::EndScopedZero();
        return false;
    }
}

namespace {

// Shared body of PlayCue3D / PlayCue3DRaw. `characterRelative` applies the
// (camera - character) shift described at PlayCue3D; Raw skips it.
bool PlayCue3DImpl(const char* resref, const Vector& worldPosition,
                   uint8_t priorityGroup, uint8_t baseVolume,
                   bool characterRelative) {
    if (!resref || !*resref) return false;
    void* exoSound = GetCExoSound();
    if (!exoSound) return false;

    uint8_t vol = EffectiveVolumeByte(baseVolume);
    if (vol == 0) return false;  // muted — skip (0 would be engine-full)
    uint8_t group = priorityGroup ? priorityGroup : GetCuePriorityGroup();

    CResRef res;
    FillResRef(res, resref);
    Vector pos = worldPosition;  // copy out before SEH frame (no aliasing)

    // Character-relative listener: shift source by (camera - character).
    // By construction listener-to-shifted equals character-to-source, so
    // we get character-relative pan/distance with the engine's untouched
    // camera listener — no effect on other engine audio.
    //
    // Skipped for Raw.
    //
    // Fail-safe on chain failure: pass raw position through.
    if (characterRelative) {
        Vector charPos, camPos;
        if (acc::engine::GetPlayerPosition(charPos) &&
            acc::engine::GetCameraPosition(camPos)) {
            pos.x += (camPos.x - charPos.x);
            pos.y += (camPos.y - charPos.y);
            pos.z += (camPos.z - charPos.z);
        }
    }

    pitch::BeginScopedZero();
    __try {
        auto fn = reinterpret_cast<PFN_Play3DOneShotSound>(
            kAddrCExoSoundPlay3DOneShotSound);
        fn(exoSound, &res,
           pos,
           /*z_offset=*/0.0f,
           group,
           /*delay_ms=*/0,
           vol,
           /*fixed_variance=*/0.0f,
           /*pitch_variance=*/0.0f);
        pitch::EndScopedZero();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pitch::EndScopedZero();
        return false;
    }
}

}  // namespace

bool PlayCue3D(const char* resref, const Vector& worldPosition,
               uint8_t priorityGroup, uint8_t baseVolume) {
    return PlayCue3DImpl(resref, worldPosition, priorityGroup, baseVolume,
                         /*characterRelative=*/true);
}

bool PlayCue3DRaw(const char* resref, const Vector& listenerSpacePosition,
                  uint8_t priorityGroup, uint8_t baseVolume) {
    return PlayCue3DImpl(resref, listenerSpacePosition, priorityGroup, baseVolume,
                         /*characterRelative=*/false);
}

}  // namespace acc::audio
