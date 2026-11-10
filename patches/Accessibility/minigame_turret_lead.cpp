// The turret's predictive lead (see minigame_turret_lead.h).
//
// The cue aims where the fighter WILL BE when a bolt arrives, as a sighted
// player pulls ahead of the target. With K1's slow bolt (300 u/s for 3 s)
// lead is unnecessary close but essential at mid and far range, where it
// exceeds the hitbox window. The intercept is solved in the gun's frame, so
// the answer is directly where to point the turret and the ship's own motion
// folds in.

#include "minigame_turret_lead.h"

#include <cmath>

namespace acc::turret_game {

namespace {

// Range gate: no lead while the fighter would stay inside its own hitbox
// during the bolt's flight (leading there only pulls the aim off the ship
// and adds velocity jitter); ramped in from kLeadGateLowFrac to
// kLeadGateHighFrac radii of drift.
constexpr float kLeadGateLowFrac  = 0.75f;
constexpr float kLeadGateHighFrac = 1.5f;

// The fighters hold about constant speed and only turn, so the curve term is
// 0.5·a·t² with a centripetal `a`. It is weighted by t², tiny on the close
// fast passes where the estimate is noisiest; capped at this flight time.
constexpr float kAccelLeadCapSeconds = 1.0f;

float Len(const Vector& a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }

// The smallest positive t with |p + v·t| = boltSpeed·t, or -1.
float InterceptTime(const Vector& p, const Vector& v, float boltSpeed) {
    const float a  = (v.x * v.x + v.y * v.y + v.z * v.z) - boltSpeed * boltSpeed;
    const float bq = 2.0f * (p.x * v.x + p.y * v.y + p.z * v.z);
    const float cq = (p.x * p.x + p.y * p.y + p.z * p.z);
    float t = -1.0f;
    if (std::fabs(a) < 1e-3f) {
        if (std::fabs(bq) > 1e-6f) t = -cq / bq;
    } else {
        const float disc = bq * bq - 4.0f * a * cq;
        if (disc >= 0.0f) {
            const float sq = std::sqrt(disc);
            const float t1 = (-bq + sq) / (2.0f * a);
            const float t2 = (-bq - sq) / (2.0f * a);
            if (t1 > 0.0f)                         t = t1;
            if (t2 > 0.0f && (t < 0.0f || t2 < t)) t = t2;
        }
    }
    return t;
}

}  // namespace

LeadSolution SolveLead(const Vector& p, const Vector* vel, const Vector* accel,
                       float radius, float boltSpeed, float boltLife) {
    LeadSolution out{p, p, 0.0f, 0.0f, 0.0f};
    if (!vel) return out;
    const Vector& v = *vel;

    const float t = InterceptTime(p, v, boltSpeed);
    if (t <= 0.0f || t > boltLife) return out;

    out.t = t;
    const float fullLead = Len(v) * t;  // drift during flight
    const float lo = kLeadGateLowFrac  * radius;
    const float hi = kLeadGateHighFrac * radius;
    if (radius > 0.0f && hi > lo) {
        out.frac = (fullLead - lo) / (hi - lo);
        if (out.frac < 0.0f)      out.frac = 0.0f;
        else if (out.frac > 1.0f) out.frac = 1.0f;
    } else {
        out.frac = 1.0f;  // no radius this tick: lead fully
    }
    const float f = out.frac;
    out.aimRel = {p.x + v.x * t * f, p.y + v.y * t * f, p.z + v.z * t * f};
    out.dist = fullLead * f;
    out.aimRelLin = out.aimRel;
    if (!accel) return out;

    // A few fixed-point flight-time iterations layer the turn term onto the
    // linear lead; it is gated like the lead and never exceeds it.
    const Vector& ac = *accel;
    float tc = t;
    Vector pred = out.aimRel;
    float corrMag = out.dist;
    for (int it = 0; it < 3; ++it) {
        const float te = (tc < kAccelLeadCapSeconds) ? tc : kAccelLeadCapSeconds;
        const float half = 0.5f * te * te * f;
        const Vector linPart = {v.x * tc * f, v.y * tc * f, v.z * tc * f};
        Vector accPart = {ac.x * half, ac.y * half, ac.z * half};
        const float lpm = Len(linPart);
        const float apm = Len(accPart);
        if (apm > lpm && apm > 1e-4f) {
            const float s = lpm / apm;
            accPart.x *= s; accPart.y *= s; accPart.z *= s;
        }
        const Vector corr = {linPart.x + accPart.x, linPart.y + accPart.y,
                             linPart.z + accPart.z};
        pred = {p.x + corr.x, p.y + corr.y, p.z + corr.z};
        corrMag = Len(corr);
        tc = Len(pred) / boltSpeed;
        if (tc > boltLife)  tc = boltLife;
        else if (tc < 0.0f) tc = 0.0f;
    }
    out.aimRel = pred;
    out.t      = tc;
    out.dist   = corrMag;
    return out;
}

}  // namespace acc::turret_game
