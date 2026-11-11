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

// The opener and the key list go out as ONE interrupting utterance, so the
// second half cannot cut off the first.
void AnnounceEntry() {
    // KOTOR 2 rebinds the race: Space is the jump and W the shift (see
    // kAddrK2RunOnAccelerateScript).
    const acc::strings::Id controls =
        AutopilotOn()           ? acc::strings::Id::SwoopRaceAutopilot
        : acc::game::IsKotor2() ? acc::strings::Id::SwoopRaceControlsK2
                                : acc::strings::Id::SwoopRaceControls;
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s %s",
                  acc::strings::Get(acc::strings::Id::SwoopRaceStarted),
                  acc::strings::Get(controls));
    acc::speech::Speak(buf, /*interrupt=*/true);
    acclog::Write("SwoopRace", "spoke entry: [%s]", buf);
}

void HandleEnter() {
    const acc::minigame::Latch latch = g_state.latch;
    g_state = State{};
    g_state.latch         = latch;
    g_state.entered_at_ms = GetTickCount64();
    // Loops left over from an earlier race must not survive into this one.
    ResetSpatialAudio();

    // Speak before logging, so the utterance is queued before the next
    // tick's cues can pile onto the same audio frame.
    AnnounceEntry();
    EmitEntrySummary(latch.game);
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
    if (!acc::game::IsKotor2() || !g_state.latch.Active()) return false;
    void* mg = g_state.latch.game;
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

bool IsActive() { return g_state.latch.Active(); }

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
    if (!g_state.latch.Active()) return false;
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
    using acc::minigame::LatchStep;
    // The turret / gunner minigame shares the struct and its vtable but
    // reports another type; minigame_turret.cpp handles it.
    switch (acc::minigame::TickLatch(g_state.latch, kMiniGameTypeSwoop, "SwoopRace")) {
        case LatchStep::Idle:    return;
        case LatchStep::Entered: HandleEnter(); return;
        case LatchStep::Exited:  HandleExit(); return;
        case LatchStep::Running: break;
    }

    // Keyboard shift: the bare W hotkey, on BOTH games so the muscle memory
    // carries between them (KOTOR 1's Space still shifts too). On KOTOR 2
    // Space (the jump) arrives as an event at the client input hook
    // (input_pipeline.cpp); W never does (the race polls it as an axis, not
    // an event), so it is polled here instead.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::SwoopShift)) {
        TryShift();
    }

    TickGearWatch(g_state.latch.game);
    TickShiftReady(g_state.latch.game);
    TickAutoShift();
    TickJumpWatch(g_state.latch.game);
    TickHitPoints(g_state.latch.game);   // log only; see DecideWreckAtFinish
    // TickRaceTimer first so have_start_ms is current this tick: it flips true
    // at the "Go!" launch (actual speed > 1 + fresh MIN_TIME stamp). The
    // obstacle and pad cues stay silent through the countdown until then.
    TickRaceTimer(g_state.latch.game);
    if (g_state.have_start_ms) {
        TickSpatialAudio(g_state.latch.game);
    }
    TickStuckHint(g_state.latch.game);
    FlushRaceEndSpeech(/*force=*/false);
    EmitDiagSnapshot(g_state.latch.game);
}

}  // namespace acc::swoop_race
