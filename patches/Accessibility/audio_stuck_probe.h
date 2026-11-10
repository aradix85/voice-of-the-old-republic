// The stuck-direction probe of audio_footstep_suppress (audio_stuck_probe.cpp).

#pragma once

#include <cstdint>

#include "engine_offsets_types.h"  // Vector

namespace acc::audio::footstep_suppress {

void TickCirclingAnnounce(const Vector& pos, uint64_t now_ms, bool walking);
void TickStuckAnnounce(const Vector& pos, uint64_t now_ms, bool walking);
void ResetProbeTriggers();

}  // namespace acc::audio::footstep_suppress
