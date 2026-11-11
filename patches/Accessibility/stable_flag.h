// A yes/no reading that only counts once it has held for a quiet time, so
// that a flicker around a change is not reported as two changes.

#pragma once

#include <cstdint>

namespace acc {

class StableFlag {
public:
    explicit constexpr StableFlag(uint32_t quietMs) : quietMs_(quietMs) {}

    // Feed the current reading with a GetTickCount-style time. True on the
    // call where the settled value changes. The first reading is adopted
    // without counting as a change.
    bool Update(bool reading, uint32_t nowMs) {
        if (!seen_) {
            seen_ = true;
            settled_ = pending_ = reading;
            changedAtMs_ = nowMs;
            return false;
        }
        if (reading != pending_) {
            pending_ = reading;
            changedAtMs_ = nowMs;
        }
        if (pending_ == settled_ || nowMs - changedAtMs_ < quietMs_) return false;
        settled_ = pending_;
        return true;
    }

    bool Settled() const { return settled_; }

private:
    uint32_t quietMs_;
    uint32_t changedAtMs_ = 0;
    bool     seen_ = false;
    bool     settled_ = false;
    bool     pending_ = false;
};

}  // namespace acc
