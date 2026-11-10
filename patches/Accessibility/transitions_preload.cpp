// The destination before a module load: the SetMoveToModuleString hooks
// (KOTOR 1 and KOTOR 2), the pre-load announcement and the module-load
// latch that keeps speech quiet through the load.

#include "transitions_internal.h"

#include "bringup_announce.h" // IsMoviePlaying — gate speech during movies
#include "engine_game.h"
#include "engine_reads.h"    // ReadCExoString
#include "log.h"
#include "strings.h"
#include "speech.h"

namespace acc::transitions {

namespace {

// A destination module resref is short printable ASCII — "endar_spire",
// "tar_m02ac", "101PER". Anything else means the argument was read in the
// wrong shape, and the two consequences of trusting it are both bad: the log
// line stops identifying the destination (the one field a stuck-transition
// report needs) and the "Loading: …" announce speaks binary noise at the
// player.
bool LooksLikeResref(const char* s) {
    if (!s || !s[0]) return false;
    bool sawAlnum = false;
    for (size_t n = 0; s[n]; ++n) {
        if (n >= 32) return false;                 // resrefs are 16 chars
        unsigned char c = static_cast<unsigned char>(s[n]);
        if (c < 0x20 || c > 0x7e) return false;    // non-printable → wrong shape
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z')) {
            sawAlnum = true;
        }
    }
    return sawAlnum;
}

// `base` is a CExoString* { char* c_string; uint32 length }.
// ReadCExoString dereferences c_string unguarded, so the guard is ours.
bool TryReadExoString(void* base, char* out, size_t outSize) {
    out[0] = '\0';
    __try {
        return acc::engine::ReadCExoString(base, /*offset=*/0, out, outSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

bool IsModuleLoadPending() {
    return g_module_load_pending;
}

void NotifyExternalLoadStarting(const char* reason) {
    // Same latch the OnSetMoveToModuleString detour sets — arms the
    // load-pending window so per-tick / hook consumers (combat narration,
    // the menu focus hooks) stay quiet while the engine rebuilds the
    // in-game GUI on a save-game load. Cleared in Tick() on the first
    // fresh area pointer, exactly like the module-transition path.
    g_module_load_pending = true;
    acclog::Write("Transition", "external load latch armed (%s)",
                  reason ? reason : "?");
}

void AnnouncePreLoadDestination(void* exoStringPtr) {
    // Latch the module-load transient unconditionally on every fire.
    // Even if the dedup window below suppresses the announce, the engine
    // is still beginning a transition — gating consumers off the announce
    // path would let the second-of-a-pair fire go un-latched and the
    // probes would resume mid-handoff. The latch is also the mod's only
    // witness that the engine ASKED for a module change: in a log it tells
    // "requested and then died" from "never requested".
    g_module_load_pending = true;

    if (!exoStringPtr) return;

    char dest[128] = {0};
    if (!TryReadExoString(exoStringPtr, dest, sizeof(dest))) {
        acclog::Write("Transition",
            "pre-load requested but string read failed (exoStr=%p) — "
            "transition is under way regardless", exoStringPtr);
        return;
    }
    if (!LooksLikeResref(dest)) {
        acclog::Write("Transition",
            "pre-load requested but destination is not resref-shaped "
            "(exoStr=%p first byte 0x%02x) — announce skipped", exoStringPtr,
            static_cast<unsigned char>(dest[0]));
        return;
    }

    // Dedup: the engine sometimes fires SetMoveToModuleString more than
    // once inside the same transition (once with the raw resref, once with
    // a normalized form). Suppress repeats of the same destination within
    // 2 s, so the announce is heard once per transition.
    static char  s_lastDest[128] = {0};
    static DWORD s_lastTick      = 0;
    DWORD now = GetTickCount();
    if (std::strncmp(s_lastDest, dest, sizeof(s_lastDest)) == 0 &&
        (now - s_lastTick) < 2000u) {
        return;
    }
    std::strncpy(s_lastDest, dest, sizeof(s_lastDest) - 1);
    s_lastDest[sizeof(s_lastDest) - 1] = '\0';
    s_lastTick = now;

    // A movie already on screen: no speech, for the same reason Tick() is
    // silent during playback. The latch is set regardless.
    if (acc::bringup_announce::IsMoviePlaying()) {
        acclog::Write("Transition",
            "pre-load -> '%s' (speech suppressed — movie foreground)", dest);
        return;
    }

    char speech[160] = {0};
    std::snprintf(speech, sizeof(speech),
                  acc::strings::Get(acc::strings::Id::FmtTransitionLoading),
                  dest);
    acc::speech::Speak(speech, /*interrupt=*/false);
    acclog::Write("Transition", "pre-load -> '%s'", dest);
}

void LatchPreLoadDestinationK2(void* exoStringPtr) {
    // Same unconditional latch as the K1 path — this is what silences the
    // in-game GUI rebuild burst over the load.
    g_module_load_pending = true;

    char dest[128] = {0};
    if (!exoStringPtr || !TryReadExoString(exoStringPtr, dest, sizeof(dest)) ||
        !LooksLikeResref(dest)) {
        acclog::Write("Transition",
            "pre-load requested (K2) but destination unreadable (exoStr=%p) — "
            "load latch armed", exoStringPtr);
        return;
    }

    // Logged, not spoken: a resref ("203TEL") is internal noise to the
    // player, and the area-name announce names the new module properly once
    // it has surfaced.
    acclog::Write("Transition", "pre-load -> '%s' (K2, latch armed, not spoken)",
                  dest);
}

}  // namespace acc::transitions

// CServerExoApp::SetMoveToModuleString — entry hook @0x004aecd0. Fires once
// per area transition with the destination module's resref CExoString*. Pre-
// load announce path; reads the resref via the dedup-and-speak helper above.
//
// **LEA-vs-MOV bug workaround** (PR-2 in docs/upstream-prs.md).
// The wrapper emits LEA (not MOV) for `source = "esp+4"` parameters, so
// `arg_addr` is the *address* of the original [esp+4] stack slot, not the
// CExoString* value at that slot. Dereference once to get the actual
// CExoString*. SEH-guarded — if the wrapper hands us a bogus address (e.g.
// stack frame mid-teardown), absorb the fault rather than crashing.
//
// `serverApp` (ECX = CServerExoApp* facade) is unused; the handler only
// needs the destination string. Kept in the signature so the parameter
// order matches the hooks.toml declaration.
extern "C" void __cdecl OnSetMoveToModuleString(void* /*serverApp*/,
                                                void* arg_addr) {
    // Only the KOTOR 1 hook routes here (with the LEA'd slot address);
    // KOTOR 2 arrives through OnSetMoveToModuleStringK2 below with the
    // CExoString* value directly.
    EnsureSpeechInitialized();

    void* exoStringPtr = nullptr;
    __try {
        exoStringPtr = *reinterpret_cast<void**>(arg_addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Transition", "pre-load arg deref faulted (arg_addr=%p)",
            arg_addr);
        return;
    }

    acc::transitions::AnnouncePreLoadDestination(exoStringPtr);
}

// CServerExoApp::SetMoveToModuleString @0x0051BEA0 on KOTOR 2, hooked at
// 0x0051BEA9 (after the prologue stored `this`). Unoptimised frame, so the
// `CExoString const&` destination is a clean frame param at [EBP+8] — a
// CExoString* value, NOT the K1 LEA'd slot address; no double-deref here.
// Offset witnesses and the history of the wrong-function hook this replaced
// are in kotor2.hooks.toml.
extern "C" __declspec(dllexport)
void __cdecl OnSetMoveToModuleStringK2(void* ebp) {
    if (!acc::game::IsKotor2()) return;
    if (!ebp) return;

    void* exoStringPtr = nullptr;
    __try {
        exoStringPtr = *reinterpret_cast<void**>(static_cast<char*>(ebp) + 8);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        exoStringPtr = nullptr;
    }

    // Latch even when the argument read fails — the engine is changing module
    // regardless, and an un-latched load is exactly the burst this prevents.
    acc::transitions::LatchPreLoadDestinationK2(exoStringPtr);
}
