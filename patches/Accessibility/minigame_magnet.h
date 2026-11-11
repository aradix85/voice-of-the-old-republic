// Sticky magnetism for the minigame aim assists: a proximity-ramped pull on
// one axis, gentle far from the target (a guide the player can steer straight
// through to pick another target) and strong on it, capped per tick so a far
// target is pulled and not yanked. Engine-free. What stays with each game: how
// the error is computed (angular for the turret, lane units for the swoop
// bike), which target, and the sign from offset to world.

#pragma once

namespace acc::minigame {

struct MagnetParams {
    float gainFar;   // pull gain at the engage edge (gentle guide)
    float gainNear;  // pull gain dead on target (sticky)
    float maxStep;   // per-tick cap on the pull, in offset units
};

// Proximity-ramped gain. `t` in [0,1]: 0 at the engage edge, 1 dead on target
// (callers clamp). t² keeps the far end gentle and the near end sticky.
float MagnetGain(float t, const MagnetParams& p);

// One tick of magnetism on a single axis. `mappedErr` is the signed
// aim-minus-target error already mapped into offset units and offset→world
// sign (so the corrective pull is simply -mappedErr): the turret passes
// sign·worldErr, swoop passes (bikeX − padX) for its 1:1 axis. Returns the new
// offset value: offsetVal − mappedErr·gain, with the step capped to ±maxStep.
float MagnetStep(float offsetVal, float mappedErr, float gain,
                 const MagnetParams& p);

}  // namespace acc::minigame
