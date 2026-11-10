// Engine-free movement verdicts on the leader's floor-plan positions: the
// windowed stuck test of footstep suppression, and the two triggers of the
// free-directions probe (no progress, circling).

#pragma once

#include <cstdint>

#include "engine_offsets_types.h"  // Vector

namespace acc::audio {

// Stuck = windowed net speed under walking pace. Wall contact is not steady
// at frame level: the engine alternates dead stops with 1-14 m/s bounce
// frames, so an instantaneous threshold flaps. NET displacement over a
// rolling 500 ms window cancels the bounce and keeps real movement. KOTOR
// has two commanded gaits (walk ~2 m/s, run ~5.4 m/s), so a sustained net
// speed well below walking means the engine is fighting an obstacle: stuck
// below 0.8 m/s, free again above 1.2 m/s. A shallow wall slide at 3-4 m/s
// keeps its footsteps; a steep grind goes silent.
class StuckWindow {
public:
    enum class Step { Seeded, Skipped, Judged };

    // Adds the position at `ms`. The first sample after a reset only seeds;
    // one within 8 ms of the last is frame noise and is skipped.
    Step Push(const Vector& pos, uint64_t ms);
    void Reset() { *this = StuckWindow{}; }

    bool     Stuck() const { return stuck_; }
    uint64_t StuckSinceMs() const { return stuckSinceMs_; }  // 0 when free
    // The last judgement, for the log.
    float    NetMetres() const { return netMetres_; }
    uint64_t SpanMs() const { return spanMs_; }
    float    Speed() const { return speed_; }

private:
    struct Sample {
        Vector   pos;
        uint64_t ms;
    };
    // 500 ms is 10-16 samples at the 30-50 ms tick; 32 leave margin. When
    // the window outruns the buffer the oldest sample judges a slightly
    // longer span, which is harmless.
    static constexpr int kCapacity = 32;

    const Sample* Reference(uint64_t now) const;

    Sample   samples_[kCapacity] = {};
    int      head_ = 0;  // next write slot
    int      count_ = 0;
    uint64_t lastMs_ = 0;
    bool     stuck_ = false;
    uint64_t stuckSinceMs_ = 0;
    float    netMetres_ = 0.0f;
    uint64_t spanMs_ = 0;
    float    speed_ = 0.0f;
};

// Walking, yet still within 0.5 m of a checkpoint after 5 s: true once per
// such episode; a 0.5 m move or a stop re-arms it.
class NoProgressTrigger {
public:
    bool Update(const Vector& pos, uint64_t ms, bool walking);
    void Reset() { *this = NoProgressTrigger{}; }

private:
    Vector   checkpoint_ = {};
    uint64_t checkpointMs_ = 0;
    bool     announced_ = false;
};

// Walking in circles, which the no-progress trigger misses because circling
// moves more than 0.5 m: over 5 s at least 5 m of path, at most 2.5 m net,
// and a path at least 2.5 times the net. True once per episode; real
// progress re-anchors and re-arms it.
class CirclingTrigger {
public:
    bool Update(const Vector& pos, uint64_t ms, bool walking);
    void Reset() { *this = CirclingTrigger{}; }

    // The last judged window, for the log.
    float    PathMetres() const { return pathMetres_; }
    float    NetMetres() const { return netMetres_; }
    uint64_t SpanMs() const { return spanMs_; }

private:
    Vector   anchor_ = {};
    uint64_t anchorMs_ = 0;
    Vector   prev_ = {};
    float    pathMetres_ = 0.0f;
    float    netMetres_ = 0.0f;
    uint64_t spanMs_ = 0;
    bool     announced_ = false;
};

}  // namespace acc::audio
