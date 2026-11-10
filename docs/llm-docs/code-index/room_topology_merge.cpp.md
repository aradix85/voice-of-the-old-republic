# room_topology_merge.cpp (280 lines)

Source: `patches/Accessibility/room_topology_merge.cpp`

Merging nav nodes into regions: union-find, the straightness of node chains, and the first two merge passes (core merge, corner fold).

## Declarations (in source order)

- L54 `int UFFind(int x)`
- L62 `void UFUnite(int a, int b)`
- L77 `void ComputeChainStraightness(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: Corridor straightness: a degree-2 node whose two edges point roughly opposite.
- L130 `void PassCoreMerge(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 1: core merge ===== Form perceptual cores by walking the nav graph once and unioning each clear, door-free edge whose endpoints are the SAME kind of thing.
- L229 `void PassCornerFold(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 1b: corner fold ===== A sharp bend (degree-2, not chainStraight) is the corner where two corridor segments meet.
