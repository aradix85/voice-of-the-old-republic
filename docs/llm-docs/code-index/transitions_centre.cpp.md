# transitions_centre.cpp (169 lines)

Source: `patches/Accessibility/transitions_centre.cpp`

When to announce a new cluster: the player's motion and the centre rule, which waits until the player is heading into the new region rather than brushing its edge.

## Declarations (in source order)

- L68 `float CentreDistance(const Vector& pos)`
- L74 `void TrackMotion(const Vector& pos)`
- L96 `void ResetMotion()`
- L102 `void ArmCentreRule(int clusterId, const acc::room_topology::ClusterInfo& info, const Vector& pos, DWORD sinceMs)`: Start the centre rule for a cluster that has just become pending.
- L117 `bool CentreRuleReady(int clusterId, const Vector& pos)`: True once the pending centre-rule cluster may speak.
