# spatial_change_detector.h (63 lines)

Source: `patches/Accessibility/spatial_change_detector.h`

Pillar 1 Triggers 1+2 — per-feature change detection.

One per-tick scan over walls (engine_area's cached perimeter edges) + objects (AreaObjectIterator filtered by the Pillar 4 six-category predicate). Maintains per-feature last_cued_distance (T1) + last_cued_at timestamp (T1/T2 shared) + foremost-in-front debounce (T2).

Trigger 1 — 360° distance-delta. Fires when |Δd| to any in-range feature exceeds settings.pillar1.distanceDeltaThresholdMeters. Walls bin into 4 character-relative sectors; closest per sector contributes, capped at trigger1MaxWallCuesPerTick. Objects fire on per-handle crossing.

Trigger 2 — foremost-in-front (±45°). Closest wall-or-object in the Front sector. A change of foremost identity (including wall→wall) fires one cue, debounced like the compass: on settle (kT2QuietMs) or, during a continuous sweep, at the kT2HeldIntervalMs cadence. Cone- clear = silence, which is the "open space ahead" signal. The incumbent foremost is sticky (kT2SwitchHysteresisMeters) so near-equidistant front walls don't thrash.

T1 and T2 carry distinct signals and run independently: T1 is the 360° distance/approach channel (silent on pure in-place rotation — it is world-frame), T2 is the front-geometry-change channel (silent on a straight approach to a single wall). A short same-surface dedup (last_cued_at) stops them double-announcing the identical wall in one beat.

The wall-cache + surface clustering subsystem (built on area change, read here per tick) lives in spatial_wall_surfaces.{h,cpp}. The public wall accessors below are thin wrappers preserved for the existing consumer set; new code can call wall_surfaces:: directly.

## Declarations (in source order)

- L43 `void Tick()`: Self-gates on player + area resolved; rebuilds the wall cache and resets per-feature state on area change.
- L51 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`: Borrowed pointer into static storage, valid for the area lifetime.
- L54 `int GetEdgeSurfaceId(int edgeIdx)`: edgeIdx into GetCachedWalls.
- L59 `bool SegmentCrossesSurface(const Vector& a, const Vector& b, Vector& outHitPoint)`: 2D segment-vs-surface intersection.

Also 1 constants or variables at namespace scope.
