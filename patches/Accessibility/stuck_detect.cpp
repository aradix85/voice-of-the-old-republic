// Movement verdicts — see stuck_detect.h.

#include "stuck_detect.h"

namespace acc::audio {

namespace {

constexpr uint64_t kWindowMs     = 500;   // the judged span
constexpr uint64_t kMinHistoryMs = 300;   // no stuck verdict on less
constexpr uint64_t kMinDeltaMs   = 8;     // closer samples are frame noise
constexpr float    kEnterSpeed   = 0.8f;  // m/s net
constexpr float    kExitSpeed    = 1.2f;  // m/s net

constexpr uint64_t kNoProgressWindowMs = 5000;
constexpr float    kProgressMetres     = 0.5f;

constexpr uint64_t kCirclingWindowMs  = 5000;
constexpr float    kCirclingMinPath   = 5.0f;  // metres: real motion first
constexpr float    kCirclingMaxNet    = 2.5f;  // metres: and got nowhere
constexpr float    kCirclingPathRatio = 2.5f;

}  // namespace

// The newest sample at least kWindowMs old, so the judged span stays near
// the window instead of growing with the buffer; the oldest when none is
// that old yet. The walk stops AT the oldest: a slot past it is stale after
// a reset, and its ancient timestamp would pass the age test.
const StuckWindow::Sample* StuckWindow::Reference(uint64_t now) const {
    const Sample* ref = nullptr;
    for (int i = 1; i < count_; ++i) {
        const Sample& s = samples_[(head_ - 1 - i + 2 * kCapacity) % kCapacity];
        ref = &s;
        if (now - s.ms >= kWindowMs) break;
    }
    return ref;
}

StuckWindow::Step StuckWindow::Push(const Vector& pos, uint64_t ms) {
    if (count_ == 0) {
        samples_[0] = {pos, ms};
        head_ = 1;
        count_ = 1;
        lastMs_ = ms;
        stuck_ = false;
        stuckSinceMs_ = 0;
        return Step::Seeded;
    }
    if (ms - lastMs_ < kMinDeltaMs) return Step::Skipped;
    lastMs_ = ms;
    samples_[head_] = {pos, ms};
    head_ = (head_ + 1) % kCapacity;
    if (count_ < kCapacity) ++count_;

    const Sample* ref = Reference(ms);
    spanMs_ = ref ? ms - ref->ms : 0;
    netMetres_ = 0.0f;
    speed_ = 0.0f;
    if (ref && spanMs_ > 0) {
        netMetres_ = DistXY(pos, ref->pos);  // height jitter is not motion
        speed_ = netMetres_ / (static_cast<float>(spanMs_) * 0.001f);
    }
    // Enter only on a full window of evidence, leave on clear progress,
    // hold in the band between.
    if (stuck_) {
        if (speed_ > kExitSpeed) {
            stuck_ = false;
            stuckSinceMs_ = 0;
        }
    } else if (spanMs_ >= kMinHistoryMs && speed_ < kEnterSpeed) {
        stuck_ = true;
        stuckSinceMs_ = ms;
    }
    return Step::Judged;
}

bool NoProgressTrigger::Update(const Vector& pos, uint64_t ms, bool walking) {
    if (!walking) {
        checkpointMs_ = 0;
        announced_ = false;
        return false;
    }
    if (checkpointMs_ == 0) {
        checkpoint_ = pos;
        checkpointMs_ = ms;
        return false;
    }
    if (ms - checkpointMs_ < kNoProgressWindowMs) return false;
    if (DistXYSq(pos, checkpoint_) >= kProgressMetres * kProgressMetres) {
        checkpoint_ = pos;
        checkpointMs_ = ms;
        announced_ = false;
        return false;
    }
    if (announced_) return false;
    announced_ = true;
    return true;
}

bool CirclingTrigger::Update(const Vector& pos, uint64_t ms, bool walking) {
    if (!walking) {
        anchorMs_ = 0;
        pathMetres_ = 0.0f;
        announced_ = false;
        return false;
    }
    if (anchorMs_ == 0) {
        anchor_ = pos;
        anchorMs_ = ms;
        prev_ = pos;
        pathMetres_ = 0.0f;
        return false;
    }
    pathMetres_ += DistXY(pos, prev_);
    prev_ = pos;
    if (ms - anchorMs_ < kCirclingWindowMs) return false;

    netMetres_ = DistXY(pos, anchor_);
    spanMs_ = ms - anchorMs_;
    const bool circling = pathMetres_ >= kCirclingMinPath &&
                          netMetres_ <= kCirclingMaxNet &&
                          pathMetres_ >= netMetres_ * kCirclingPathRatio;
    if (!circling) {  // real progress: re-anchor and re-arm
        anchor_ = pos;
        anchorMs_ = ms;
        pathMetres_ = 0.0f;
        announced_ = false;
        return false;
    }
    if (announced_) return false;  // the anchor holds until the player escapes
    announced_ = true;
    return true;
}

}  // namespace acc::audio
