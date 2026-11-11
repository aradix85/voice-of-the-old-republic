#include "minigame_latch.h"

namespace acc::minigame {

LatchStep StepLatch(Latch& latch, const LatchInput& in) {
    const bool ours = in.areaGame && in.areaIsOurs;
    if (!latch.Active()) {
        if (!ours) return LatchStep::Idle;
        latch = Latch{in.areaGame, in.areaVtable, 0};
        return LatchStep::Entered;
    }

    if (in.areaGame && in.areaGame != latch.game) {
        if (ours) {
            latch = Latch{in.areaGame, in.areaVtable, 0};
            return LatchStep::Running;
        }
        ++latch.ticksLost;  // another kind of minigame has the area
    } else if (in.areaGame || in.latchedAlive) {
        // The area agrees with the latch, or it dropped to null (the start
        // transition does this) while the latched struct is still intact.
        latch.ticksLost = 0;
    } else {
        ++latch.ticksLost;  // both say gone
    }

    if (latch.ticksLost < kExitDebounceTicks) return LatchStep::Running;
    latch = Latch{};
    return LatchStep::Exited;
}

}  // namespace acc::minigame
