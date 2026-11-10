# minigame_turret_lead.cpp (113 lines)

Source: `patches/Accessibility/minigame_turret_lead.cpp`

The turret's predictive lead (see minigame_turret_lead.h).

The cue aims where the fighter WILL BE when a bolt arrives, as a sighted player pulls ahead of the target. With K1's slow bolt (300 u/s for 3 s) lead is unnecessary close but essential at mid and far range, where it exceeds the hitbox window. The intercept is solved in the gun's frame, so the answer is directly where to point the turret and the ship's own motion folds in.

## Declarations (in source order)

- L30 `float Len(const Vector& a)`
- L33 `float InterceptTime(const Vector& p, const Vector& v, float boltSpeed)`: The smallest positive t with |p + v·t| = boltSpeed·t, or -1.
- L55 `LeadSolution SolveLead(const Vector& p, const Vector* vel, const Vector* accel, float radius, float boltSpeed, float boltLife)`
