#include "minigame_magnet.h"

namespace acc::minigame {

float MagnetGain(float t, const MagnetParams& p) {
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    // t² keeps the far end gentle (a guide) and the near end strong (sticky).
    return p.gainFar + t * t * (p.gainNear - p.gainFar);
}

float MagnetStep(float offsetVal, float mappedErr, float gain,
                 const MagnetParams& p) {
    float step = -mappedErr * gain;
    if (step >  p.maxStep) step =  p.maxStep;
    else if (step < -p.maxStep) step = -p.maxStep;
    return offsetVal + step;
}

}  // namespace acc::minigame
