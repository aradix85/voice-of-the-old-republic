# minigame_turret_lead.h (28 lines)

Source: `patches/Accessibility/minigame_turret_lead.h`

The turret's predictive lead: where to point the gun so a bolt fired now meets the fighter. Engine-free.

## Declarations (in source order)

- L10 `struct LeadSolution`
- L24 `LeadSolution SolveLead(const Vector& p, const Vector* vel, const Vector* accel, float radius, float boltSpeed, float boltLife)`: `p` is the fighter relative to the gun.
