# spatial_change_detector.cpp (1229 lines)

Source: `patches/Accessibility/spatial_change_detector.cpp`

routes T2 cone through camera yaw (where the user is looking) in both normal play and view mode

## Declarations (in source order)

- L188 `bool ForemostEqual(const Foremost& a, const Foremost& b)`
- L198 `const char* ForemostKindTag(FeatureKind k)`
- L217 `float T2EffectiveDistance(const Foremost& cand, float rawDist)`: Effective comparison distance for T2 foremost selection.
- L230 `float ClosestPointDistanceSquared(const Vector& p, const Vector& a, const Vector& b, Vector& outClosest)`: Closest-point-on-segment squared distance.
- L255 `acc::audio::NavCue ClosedDoorCueForMaterial(void* obj)`: Closed-door material-specific cue.
- L268 `acc::audio::NavCue CategoryToNavCue(acc::filter::CycleCategory c, void* obj)`: Map a category to its NavCue.
- L284 `bool ClassifyObject(void* obj, acc::audio::NavCue& outCue)`
- L297 `ObjectState* FindOrAddObjectState(uint32_t handle, bool& outIsNew)`
- L315 `ObjectState* FindObjectState(uint32_t handle)`
- L323 `void RemoveObjectState(uint32_t handle)`
- L410 `WallSector ClassifyRelativeBearing(float relBearingDeg)`
- L423 `const char* SectorTag(WallSector s)`: Tag used in T1 logs — world-frame quadrant labels.
- L450 `void OnAreaChange(void* area)`
- L498 `void CalibrateInRange(void* area, const Vector& referencePos, const char* reason)`: Calibration scan — runs once per area-change after OnAreaChange has rebuilt the cache.
- L617 `void Tick()`
- L1208 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`
- L1216 `int GetEdgeSurfaceId(int edgeIdx)`
- L1223 `bool SegmentCrossesSurface(const Vector& a, const Vector& b, Vector& outHitPoint)`
