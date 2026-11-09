// Spots on the floor plan as the exit memory keeps them in save variables:
// whole metres, "x,y" or "x,y=value", joined by ';'. ACC_VISIT_<area> holds
// the centres of visited regions, ACC_EXIT_<area> door spots with the module
// each door leads to. Engine-free, so the unit tests cover the format.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "engine_offsets_types.h"  // Vector, DistXYSq

namespace acc::save_points {

struct Spot {
    long        x = 0;
    long        y = 0;
    std::string value;  // empty: a plain spot
};

// `p` rounded to whole metres, with `value`.
Spot At(const Vector& p, const std::string& value = std::string());

// The spots in `text`, up to the first malformed entry. "x,y=" with an empty
// value is skipped.
std::vector<Spot> Parse(const char* text);

// The text Parse reads back.
std::string Format(const std::vector<Spot>& spots);

// The spot nearest `p` within `maxM` metres, or null.
const Spot* Nearest(const std::vector<Spot>& spots, const Vector& p, float maxM);

// Puts `s` last: drops the spots within `sameM` metres of it, then the
// oldest ones beyond `max` spots in all.
void Put(std::vector<Spot>& spots, const Spot& s, long sameM, size_t max);

}  // namespace acc::save_points
