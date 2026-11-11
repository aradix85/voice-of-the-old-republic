// Which minigame of ours is running. The swoop race and the turret game are
// both a CSWMiniGame that the player's area points at; each module latches
// the struct at entry and keeps it through the moments the area stops
// pointing at it. Engine-free: minigame_engine reads the two truth sources.

#pragma once

namespace acc::minigame {

// Ticks both truth sources must say "gone" in a row before the minigame
// counts as over. The start transition flips the area chain for fewer than
// five ticks, so 60 (about 2 s at 30 fps) is well past that without making a
// true exit feel late.
constexpr int kExitDebounceTicks = 60;

struct Latch {
    void* game      = nullptr;  // the latched CSWMiniGame; null when idle
    void* vtable    = nullptr;  // its vtable at latch time
    int   ticksLost = 0;        // ticks in a row that both sources said gone

    bool Active() const { return game != nullptr; }
};

// What one tick sees.
struct LatchInput {
    void* areaGame     = nullptr;  // the area's minigame of any kind, or null
    void* areaVtable   = nullptr;  // its vtable
    bool  areaIsOurs   = false;    // it is the kind this latch is for
    // The latched struct still carries the vtable it was latched with: the
    // engine frees it at the end and the heap slot tends to be reused.
    bool  latchedAlive = false;
};

enum class LatchStep {
    Idle,     // nothing of ours
    Entered,  // latched this tick
    Running,  // still latched; a new struct of ours mid-game is re-latched
    Exited,   // gone for kExitDebounceTicks; the latch is cleared
};

LatchStep StepLatch(Latch& latch, const LatchInput& in);

}  // namespace acc::minigame
