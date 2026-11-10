// DLL entry + lazy-init plumbing.
//
// DllMain runs under the loader lock — no LoadLibrary, no COM init, no
// file I/O here. Speech is initialised lazily on the first hook fire, on
// both games.

#include <windows.h>
#include <cstdio>
#include <cstring>

#include "bringup_announce.h"
#include "focus_guard.h"
#include "diag_settings.h"
#include "engine_game.h"
#include "engine_input.h"
#include "engine_keyboard_resync.h"
#include "engine_keymap.h"
#include "log.h"
#include "mod_version.h"
#include "speech.h"
#include "save_crash_guard.h"
#include "strings.h"
#include "engine_rebase.h"

namespace {

char g_versionSha[128] = "(unset)";

}  // namespace

// First hook to fire runs it; subsequent calls are no-ops. Speaks a
// "loaded, version X" greeting so the user knows the patch is active.
void EnsureSpeechInitialized() {
    static bool done = false;
    if (done) return;
    done = true;
    if (acc::speech::Init()) {
        char greeting[128];
        snprintf(greeting, sizeof(greeting),
                 acc::strings::Get(acc::strings::Id::FmtModLoadedVersion),
                 acc::kModVersion);
        acc::speech::Speak(greeting, /*interrupt=*/true);
    }
}

// Everything the patch has to stand up once per session, in the order that
// matters. Idempotent and game-agnostic.
//
// It lives in its own function because the two games reach it from different
// places. KOTOR 1 calls it from OnRulesInit, the CSWRules constructor detour.
// KOTOR 2 has no rules-init hook (there is no such entry in
// kotor2.hooks.toml), so it calls it from the first engine tick — see
// core_tick.cpp. Before that call existed, every line below was KOTOR 1 only,
// and several of them are subsystems that KOTOR 2 code *consumes* without ever
// having started: the focus probe is the producer of every
// RequestInputReacquire / RequestInputRelease, so its absence made the tick's
// DrainPendingReacquire a permanent no-op on KOTOR 2 and left the focus-theft
// recovery missing there even after its SetActive constants were resolved.
//
// Everything here is past the loader lock at both call sites, which the file
// I/O and the two worker threads below require.
void EnsureSessionBringup() {
    static bool done = false;
    if (done) return;
    done = true;

    // No-mouse DirectInput crash guard. On KOTOR 2 this has already run from
    // DllMain (the crash can land before the first tick); the call is
    // idempotent, and on KOTOR 1 this is where it installs — after the
    // engine's first successful mouse init, the timing the shipped fix was
    // validated against.
    acc::engine::InstallDirectInputMouseGuard();
    // Keep a failed mouse device from taking the keyboard with it. KOTOR 1
    // installs it HERE rather than from DllMain (where KOTOR 2 does) for the
    // same reason nothing else patches KOTOR 1's .text that early: the Steam
    // build is packed, and at DllMain — which runs from a static import, before
    // the entry point — the bytes we scan for are still encrypted, so the scan
    // would match nothing. Idempotent, so the KOTOR 2 call below is not undone
    // by this one.
    acc::engine::InstallMouseTeardownBlock();
    // Keyboard transition-stream re-sync: a release the engine discards (its
    // own SetActive edges, a lost DirectInput device) is re-delivered instead
    // of stranding the key. KOTOR 1 installs here for the same packed-.text
    // reason as the teardown block; KOTOR 2 has already done so from DllMain.
    acc::engine::InstallKeyboardStreamResync();
    // Save-thumbnail divide-by-zero guard. Declines on KOTOR 2 by design —
    // see the note at its install site.
    acc::save_guard::InstallSaveScreenshotGuard();
    EnsureSpeechInitialized();
    // Baseline snapshot of the game ini + install-root DLLs so every support
    // bundle from now on carries the user's full config without needing a
    // follow-up "what's in your ini?" round-trip. Picks swkotor2.ini on
    // KOTOR 2 — it was already per-game aware, it was simply never called
    // there, which is why KOTOR 2 support bundles carried no config at all.
    acc::diag::settings::LogStartupSnapshot();
    // Build the engine keybinding table (hardcoded command -> scancode -> VK,
    // resolved against the active keyboard layout) so the input hooks can
    // swallow the engine's bare-key action when a modifier-using mod hotkey
    // shadows an engine-bound key. The shadowed gameplay hotkeys are hardcoded
    // in the engine (not in the rebindable Key Mapping screen), so this is a
    // one-time build per session. VksForCode self-heals via a lazy Rebuild if
    // it is asked first, so this call is about doing it once, up front, with
    // the table dumped to the log where support can read it.
    acc::engine_keymap::Rebuild();
    // Spin up the focus-probe polling thread now (rather than at
    // MainMenu first-sight) so we catch focus events during intro-movie
    // playback + the SWMovieWindow → Render Window handoff that
    // happens before the main menu shows. Idempotent — re-call from
    // MainMenu first-sight is a no-op.
    acc::focus_guard::StartFocusProbe();
    // Bringup-phase nag — speaks "Game is still loading" once if the
    // user presses an arrow / Enter / Space during the post-intro
    // pre-pump-live window. Silent during movies + after pump live. Its phase
    // machine keys on the "SWMovieWindow" and "Render Window" class names,
    // both of which are present in the KOTOR 2 binary.
    acc::bringup_announce::Start();
}

// CSWRules::CSWRules construction detour (hooks.toml @ 0x00552c9a).
// First fire is the "patch alive" signal and speech-init trigger. File
// I/O is safe here
// (we're well past loader lock by the time CSWRules runs).
extern "C" void __cdecl OnRulesInit(void* /*rulesThis*/) {
    // KOTOR 2 never reaches this: it has no rules-init hook to install. The
    // gate stays as a belt-and-braces assertion of that.
    if (!acc::game::HandlerEnabled()) return;
    static bool fired = false;
    if (fired) return;
    fired = true;
    acclog::BringupMark("rules_init");
    EnsureSessionBringup();
    acclog::Write("Init", "first CSWRules construction; detour active");
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        acclog::Init(hinstDLL);
        acclog::BringupMark("dll_attach");
        DWORD n = GetEnvironmentVariableA(
            "KOTOR_VERSION_SHA", g_versionSha, sizeof(g_versionSha));
        if (n == 0 || n >= sizeof(g_versionSha)) {
            strncpy_s(g_versionSha, "(unset)", _TRUNCATE);
        }
        acclog::Write("Init", "DLL_PROCESS_ATTACH sha=%s", g_versionSha);
        // Which game/build, logged beside the SHA because they answer the same
        // question from two directions: the SHA is what KotorPatcher matched,
        // this is what we independently detected from the game's PE header. A
        // disagreement between the two lines is a real bug and worth spotting.
        //
        // Safe under loader lock: detection is a walk of the already-mapped
        // image, with no file I/O and no allocation (see engine_game.h). It is
        // deliberately here rather than in the startup snapshot so it appears
        // even when NO hooks are installed — which is exactly the state the
        // KOTOR 2 bring-up runs in.
        acclog::Write("Game.Identity", "title=%s build=%s",
                      acc::game::TitleName(), acc::game::BuildName());
        // No-mouse crash guard, KOTOR 2 only. It has to go here for the same
        // reason the identity line above does: KOTOR 2 has no rules-init hook,
        // and this must be in place even in a session where no hook ever fires.
        // The guard cannot be deferred to the first hook that does fire — the
        // crash lands on the first main-loop frame that updates the mouse,
        // which on a no-mouse machine can precede any menu panel we hook.
        //
        // Loader-lock safe: one VirtualAlloc, one VirtualProtect, a memcpy and
        // a FlushInstructionCache. No LoadLibrary, no COM, nothing re-entrant.
        //
        // Ordering is a non-issue. The trampoline only diverges when the
        // engine's DirectInput interface is already NULL, which is a state the
        // engine reaches on its own and never recovers from — so installing
        // before the engine's own DirectInput bring-up is harmless: a
        // with-mouse player's interface is non-NULL and the guard falls
        // straight through to the original code.
        if (acc::game::IsKotor2()) {
            acc::engine::InstallDirectInputMouseGuard();
            // Same reasoning, one step upstream: the guard keeps a torn-down
            // DirectInput from crashing us, this keeps the mouse from tearing
            // it down at all. It has to be as early as the guard, because the
            // teardown it blocks happens on the same first mouse-polling frame
            // the crash used to. KOTOR 2's image is not packed, so the
            // verification scan reads real bytes here.
            acc::engine::InstallMouseTeardownBlock();
            // Keyboard transition-stream re-sync (engine_keyboard_resync.h).
            // A static CALL-site rewrite inside GetEvents; nothing about it
            // depends on the engine's input bring-up having run, and the
            // unpacked image makes the site verifiable here.
            acc::engine::InstallKeyboardStreamResync();
        }
        // Speech init deferred — loader lock.
    }
    return TRUE;
}
