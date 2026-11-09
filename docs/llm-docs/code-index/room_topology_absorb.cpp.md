# room_topology_absorb.cpp (451 lines)

Source: `patches/Accessibility/room_topology_absorb.cpp`

The later merge passes: small leftover nodes are absorbed into the region next to them (adjacent stragglers, stragglers inside a region's bounding box, open-ended pendants).

## Declarations (in source order)

- L85 `void AbsorbAdjacentStragglers(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 2: straggler absorb ===== Fold leftover corner / doorway nodes into the core they belong to.
- L198 `void AbsorbStragglersByBoundingBox(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: (b) Bounding-box admission for remaining graph-unattached singletons.
- L356 `void AbsorbOpenEndedPendants(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`
