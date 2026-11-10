// The free-directions probe: when the leader walks without getting anywhere,
// or walks in circles (stuck_detect.h), speak once which of the eight
// compass directions are free of walls and bodies for 1.5 m.

#pragma once

#include <cstdint>

#include "engine_offsets_types.h"  // Vector

namespace acc::audio::stuck_probe {

// Per judged position sample, from footstep_suppress::Tick. `walking`: a
// fresh leader footstep, i.e. the walk animation runs.
void Tick(const Vector& pos, uint64_t now_ms, bool walking);

// No player: both triggers start over.
void Reset();

}  // namespace acc::audio::stuck_probe
