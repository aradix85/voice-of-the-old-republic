// Health warnings in a fight: the band a creature's health is in, and per
// creature the band it was last told about. Engine-free.

#pragma once

#include <cstdint>
#include <map>

namespace acc::combat {

// 0 above half, 1 at half or below, 2 at a quarter or below. A creature at
// zero is down, which the game announces itself: band 0.
inline int HealthBand(int hp, int maxHp) {
    if (hp <= 0 || maxHp <= 0) return 0;
    if (hp * 4 <= maxHp) return 2;
    if (hp * 2 <= maxHp) return 1;
    return 0;
}

class HealthWarnings {
public:
    void Clear() { told_.clear(); }

    // The band `id` is in now, when that is deeper than the band it was last
    // seen in; 0 otherwise. Healing out of a band makes it tellable again.
    int Sank(uint32_t id, int hp, int maxHp) {
        const int band = HealthBand(hp, maxHp);
        int& told = told_[id];
        const bool deeper = band > told;
        told = band;
        return deeper ? band : 0;
    }

private:
    std::map<uint32_t, int> told_;
};

}  // namespace acc::combat
