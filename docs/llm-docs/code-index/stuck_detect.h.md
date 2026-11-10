# stuck_detect.h (98 lines)

Source: `patches/Accessibility/stuck_detect.h`

Engine-free movement verdicts on the leader's floor-plan positions: the windowed stuck test of footstep suppression, and the two triggers of the free-directions probe (no progress, circling).

## Declarations (in source order)

- L21 `class StuckWindow`: Stuck = windowed net speed under walking pace.
- L62 `class NoProgressTrigger`: Walking, yet still within 0.5 m of a checkpoint after 5 s: true once per such episode; a 0.5 m move or a stop re-arms it.
- L77 `class CirclingTrigger`: Walking in circles, which the no-progress trigger misses because circling moves more than 0.5 m: over 5 s at least 5 m of path, at most 2.5 m net, and a path at least 2.5 times the net.
