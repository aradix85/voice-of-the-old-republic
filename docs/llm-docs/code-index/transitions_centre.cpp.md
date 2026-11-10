# transitions_centre.cpp (114 lines)

Source: `patches/Accessibility/transitions_centre.cpp`

When to announce a new cluster: the player's motion and the centre rule, which waits until the player is heading into the new region rather than brushing its edge.

## Declarations (in source order)

- L29 `void TrackMotion(const Vector& pos)`
- L51 `void ResetMotion()`
- L56 `float CentreDistance(const Vector& pos)`
- L62 `bool CentreRuleReady(int clusterId, const Vector& pos)`: True once the pending centre-rule cluster may speak.
