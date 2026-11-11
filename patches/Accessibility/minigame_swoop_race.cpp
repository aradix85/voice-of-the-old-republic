// Swoop race: entry and exit, the shift and jump keys, and the per-tick
// dispatch to the cues, the race clock and the spatial audio.
//
// The CSWMiniGame pointer is latched on first detect: the area chain
// (GetCurrentArea -> GetClientArea -> mini_game) stops resolving during the
// race-start transition while the race runs on, so the chain alone would
// announce a false exit a few ticks after the entry.

#include "minigame_swoop_race_internal.h"

#include "engine_area.h"
#include "engine_game.h"
#include "engine_keymap.h"
#include "engine_addr.h"
#include "hotkeys.h"
#include "key_inject.h"
#include "log.h"
#include "speech.h"
#include "strings.h"

namespace acc::swoop_race {

namespace {

// CSWCArea.mini_game (swkotor.exe.h line 8726, after rooms@+0x260).
//
// KOTOR 2 = +0x268. Witnessed as a WRITE, not inferred: the K2 area's
// minigame-load path @0x007A2FB0 allocates 0x11c bytes, runs the CSWMiniGame
// ctor @0x00839260 on it, and stores the result with `mov [ecx+0x268], edx`;
// the reader at 0x007AA940 loads the same slot. The +4 shift matches the map-pin
// array's (kClientAreaMapPinsOffset, 0x1c4 -> 0x1c8) — CSWCArea gained one dword
// low down and everything above moved together. Same delta again at the tail of
// the CSWMiniGame ctor, which takes the area's resref from area+0x110 on K2
// where KOTOR 1 takes it from +0x10c.
const size_t kClientAreaMiniGameOffset      = acc::off::Pick(0x264, 0x268);

// KOTOR 2 ONLY — the two per-slot script runners the client input handler
// (CClientExoAppInternal::HandleInputEvent twin @0x007b12c0) calls on the
// player bike. 173-byte twins of KOTOR 1's CSWMiniGameObject::OnHitFollower
// @0x0066c2d0: `if (param == 0) { GetScriptName(slot) -> valid? -> RunScript
// (name, mgo_index, 1) }`, with the slot index a literal:
//   @0x00838e10 -> slot 10 = OnAccelerate  (the gear ladder, onaccelerate.ncs)
//   @0x00838ec0 -> slot 11 = OnBrake       (the jump, onjump.ncs)
// The engine reaches them from event ids 0x16 (pad B) / 0x19 (left mouse
// button) for OnAccelerate and 0x1a (right mouse button) / 0xd9 (the MGshoot
// keymap row = Space) for OnBrake — events registered by K2's
// SetEventDescriptions twin @0x00785df0 (0x19 <- InputIndex 0 MOUSE_BUTTON0,
// 0x1a <- InputIndex 1 MOUSE_BUTTON1, class 1). KOTOR 1 sends 0x19 and 0xd9
// to CSWMiniPlayer::Fire instead, whose gun bank runs onfire.ncs — so Space
// shifts there and no keyboard key shifts on KOTOR 2 by default. The mod
// therefore routes Space -> TryJump (logged) and W -> TryShift.
// __thiscall with one int: the typedef must carry the int, or the callee
// reads caller-frame garbage.
const uintptr_t kAddrK2RunOnAccelerateScript = acc::addr::Kotor2Only(0x00838e10);
const uintptr_t kAddrK2RunOnBrakeScript      = acc::addr::Kotor2Only(0x00838ec0);
typedef void (__thiscall* PFN_RunFollowerSlotScript)(void* follower, int flag);

// KOTOR 1's shift input: [Keymapping] Action217 = keymap.2da row 17 MGshoot
// (event 0xd9), Space when the ini has no line for it. See TryShift.
constexpr int kK1MGShootActionId = 217;
constexpr int kDikSpace          = 0x39;

// CSWCArea.mini_game via the player-area chain. Ground truth at the moment of
// detection; the result is latched so churn in this chain during transitions
// does not drop the race.
void* ReadMiniGameViaArea() {
    void* serverArea = acc::engine::GetCurrentArea();
    if (!serverArea) return nullptr;
    void* clientArea = acc::engine::GetClientArea(serverArea);
    return ReadPtrOrNull(clientArea, kClientAreaMiniGameOffset);
}

// True iff the latched CSWMiniGame is still readable and its vtable is the
// one captured at latch time. The engine frees CSWMiniGame at race end and
// the heap slot tends to get reused, so a vtable mismatch is a reliable
// "no longer the same struct" signal.
bool LatchedStillValid() {
    if (!g_state.latched_mini_game || !g_state.latched_vtable) return false;
    void* vt = ReadPtrOrNull(g_state.latched_mini_game, kMiniGameVtableOffset);
    return vt == g_state.latched_vtable;
}

// The opener and the key list go out as ONE interrupting utterance, so the
// second half cannot cut off the first.
void AnnounceEntry() {
    // KOTOR 2 rebinds the race: Space is the jump and W the shift (see
    // kAddrK2RunOnAccelerateScript).
    const acc::strings::Id controls = acc::game::IsKotor2()
        ? acc::strings::Id::SwoopRaceControlsK2
        : acc::strings::Id::SwoopRaceControls;
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s %s",
                  acc::strings::Get(acc::strings::Id::SwoopRaceStarted),
                  acc::strings::Get(controls));
    acc::speech::Speak(buf, /*interrupt=*/true);
    acclog::Write("SwoopRace", "spoke entry: [%s]", buf);
}

void HandleEnter(void* mg) {
    g_state = State{};
    g_state.active            = true;
    g_state.latched_mini_game = mg;
    g_state.latched_vtable    = ReadPtrOrNull(mg, kMiniGameVtableOffset);
    g_state.entered_at_ms     = GetTickCount64();
    // Loops left over from an earlier race must not survive into this one.
    ResetSpatialAudio();

    // Speak before logging, so the utterance is queued before the next
    // tick's cues can pile onto the same audio frame.
    AnnounceEntry();
    EmitEntrySummary(mg);
}

void HandleExit() {
    acclog::Write("SwoopRace", "EXIT after %llu ms (debounced %d ticks)",
                  GetTickCount64() - g_state.entered_at_ms, kExitDebounceTicks);
    // Objects may still be in range when the race ends.
    ResetSpatialAudio();
    // A no-op when the finish detector already spoke the race end.
    SpeakRaceEndMessage();
    FlushRaceEndSpeech(/*force=*/true);   // a delayed time must not die with the module
    g_state = State{};
}

// Run one of the K2 player bike's slot scripts through the engine's own
// runner, with the guards the client input handler applies (a live
// swoop-type minigame with a player). False = nothing ran.
bool RunPlayerSlotScript(uintptr_t runner, const char* what) {
    if (!acc::game::IsKotor2() || !g_state.active) return false;
    void* mg = g_state.latched_mini_game;
    if (!mg || runner == 0) return false;
    if (ReadU32Or0(mg, kMiniGameTypeOffset) != kMiniGameTypeSwoop) return false;
    void* player = ReadPtrOrNull(mg, kMiniGamePlayerOffset);
    if (!player) return false;
    __try {
        reinterpret_cast<PFN_RunFollowerSlotScript>(runner)(player, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("SwoopRace", "%s: runner faulted (player=%p)", what, player);
        return false;
    }
    acclog::Write("SwoopRace", "%s: ran (gear global=%d)", what,
                  acc::engine::ReadGlobalNumber(kGearGlobalName));
    return true;
}

}  // namespace

bool IsActive() { return g_state.active; }

bool TryShift() {
    if (acc::game::IsKotor2()) {
        return RunPlayerSlotScript(kAddrK2RunOnAccelerateScript, "shift (OnAccelerate)");
    }
    // KOTOR 1: the shift is CSWMiniPlayer::Fire (the gun bank runs
    // onfire.ncs, the gear ladder), which HandleInputEvent @0x00621210 calls
    // on event 0xd9 — the MGshoot keymap row, Action217, Space by default.
    // Pressed through the player's own bind rather than by calling Fire, so
    // the engine's guards on that case (module + area present, not paused)
    // stay the engine's. The press reaches the client input hook as 0xd9,
    // where TryJump declines on KOTOR 1 and the engine shifts.
    if (!g_state.active || !g_state.latched_mini_game) return false;
    int scan = acc::engine_keymap::GameActionScancode(kK1MGShootActionId);
    if (scan == 0) scan = kDikSpace;
    acc::key_inject::Tap(scan);
    acclog::Write("SwoopRace", "shift (MGshoot key, scan 0x%02x): injected "
                  "(gear global=%d)", scan,
                  acc::engine::ReadGlobalNumber(kGearGlobalName));
    return true;
}

bool TryJump() {
    return RunPlayerSlotScript(kAddrK2RunOnBrakeScript, "jump (OnBrake)");
}

void Tick() {
    void* mgArea = ReadMiniGameViaArea();

    if (!g_state.active) {
        // Enter only for a swoop race (type 1). The turret / gunner minigame
        // shares this struct and vtable but reports type 2 and is handled by
        // minigame_turret.cpp. The type is set by the time the area chain
        // exposes mini_game.
        if (mgArea && ReadU32Or0(mgArea, kMiniGameTypeOffset) == kMiniGameTypeSwoop) {
            HandleEnter(mgArea);
        }
        return;
    }

    // Two truth sources: the area chain still reports a mini_game (the same
    // pointer or a fresh one), and the latched pointer's vtable still reads
    // the expected value. Exit only when BOTH say "gone" for
    // kExitDebounceTicks in a row.
    if (mgArea && mgArea != g_state.latched_mini_game) {
        // A different minigame mid-race (engine swap, injected reset):
        // re-latch so later reads track it.
        void* vt = ReadPtrOrNull(mgArea, kMiniGameVtableOffset);
        acclog::Write("SwoopRace",
                      "re-latch: old=%p new=%p vtable=%p",
                      g_state.latched_mini_game, mgArea, vt);
        g_state.latched_mini_game = mgArea;
        g_state.latched_vtable    = vt;
        g_state.ticks_since_lost  = 0;
    } else if (mgArea || LatchedStillValid()) {
        // The chain agrees with the latch, or it dropped to null (the
        // race-start transition does this) while the latched struct is
        // still the one from the entry.
        g_state.ticks_since_lost = 0;
    } else {
        // Both say gone; wait out a transient reallocation first.
        ++g_state.ticks_since_lost;
        if (g_state.ticks_since_lost >= kExitDebounceTicks) {
            HandleExit();
            return;
        }
    }

    if (!g_state.latched_mini_game) return;

    // Keyboard shift: the bare W hotkey, on BOTH games so the muscle memory
    // carries between them (KOTOR 1's Space still shifts too). On KOTOR 2
    // Space (the jump) arrives as an event at the client input hook
    // (input_pipeline.cpp); W never does (the race polls it as an axis, not
    // an event), so it is polled here instead.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::SwoopShift)) {
        TryShift();
    }

    TickGearWatch(g_state.latched_mini_game);
    TickShiftReady(g_state.latched_mini_game);
    TickJumpWatch(g_state.latched_mini_game);
    TickHitPoints(g_state.latched_mini_game);   // log only; see DecideWreckAtFinish
    // TickRaceTimer first so have_start_ms is current this tick: it flips true
    // at the "Go!" launch (actual speed > 1 + fresh MIN_TIME stamp). The
    // obstacle and pad cues stay silent through the countdown until then.
    TickRaceTimer(g_state.latched_mini_game);
    if (g_state.have_start_ms) {
        TickSpatialAudio(g_state.latched_mini_game);
    }
    TickStuckHint(g_state.latched_mini_game);
    FlushRaceEndSpeech(/*force=*/false);
    EmitDiagSnapshot(g_state.latched_mini_game);
}

}  // namespace acc::swoop_race
