# transitions.cpp (1895 lines)

Source: `patches/Accessibility/transitions.cpp`

region labels (nav-graph decomposition)

## Declarations (in source order)

- L191 `void TrackMotion(const Vector& pos)`
- L213 `void ResetMotion()`
- L218 `float CentreDistance(const Vector& pos)`
- L224 `bool CentreRuleReady(int clusterId, const Vector& pos)`: True once the pending centre-rule cluster may speak.
- L339 `void SpeakArea(void* area)`
- L370 `void RebuildLandmarkCache(void* area)`
- L490 `bool IsFlapRepeat(const std::string& text, const Vector& pos, DWORD now)`: True when speaking `text` at `pos` would be the flap return-leg.
- L498 `void CommitSpokenLabel(const std::string& text, const Vector& pos, DWORD now)`: Rotate the two-deep spoken-label history.
- L572 `void RecomputeLandmarkRanges()`
- L646 `bool ResolveRoomSpeech(void* area, const Vector& worldPos, std::string& outBuf, const char*& outSource, int* outClusterIdOpt = nullptr, std::vector<acc::room_topology::ExitMark>* outExitsOpt = nullptr)`: Resolve the speech at `worldPos` using a two-tier order: 1.
- L721 `void LogWallTopoComparison(void* area, int roomIndex, const Vector& worldPos, const char* spoken, const char* source)`: Diagnostic — log room_topology's resolution at this position alongside whatever the resolved speech was (room name, shape, or transition compose).
- L766 `void SpeakPlayerRegionLabel(void* area, const std::string& label, const std::vector<acc::room_topology::ExitMark>& exits)`: Speak a perceptual-region change.
- L773 `void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos)`
- L881 `void TickPendingPlatz(void* area, const Vector& playerPos)`: Fire any pending Platz announce whose delay has elapsed.
- L961 `void TickGatedClusterRefire(void* area, const Vector& playerPos)`: Refire the current room label after a gated cluster change, once the gate (combat / blocking UI) has stayed clear for kGatedRefireDelayMs.
- L990 `bool TryAutoDiscoverMapNote(void* waypoint, const Vector& playerPos)`: If `waypoint` (a landmark whose note is still disabled) sits within auto-discover range of the leader, enable it via the engine's own field write.
- L1018 `void TickLandmarkCacheRecheck(void* area, const Vector& playerPos)`: Detect map notes whose enabled flag flipped since the last cache scan and rebuild.
- L1062 `void TickProximityLandmarks(const Vector& playerPos)`: Per-tick scan over the landmark cache.
- L1152 `bool IsWorldSpeechGatedImpl()`: Forward declaration of the public predicate (defined at file scope below namespace).
- L1161 `bool IsResrefStyleRoomName(const char* name)`
- L1183 `bool FindLandmarkNear(const Vector& pos, float rangeM, char* nameOut, size_t nameBufSize, Vector& posOut, int* outLandmarkIdx)`
- L1211 `bool IterateLandmarks(void* area, int& cursor, char* nameOut, size_t nameBufSize, Vector& posOut, int& outLandmarkIdx)`
- L1241 `void MarkLandmarkClaimedByDoor(int landmarkIdx)`
- L1260 `void ResetRoomSpeechState()`: Cluster / room-label speech, including the text-equality dedup key and the flap-suppression window.
- L1275 `void ResetLandmarkProximityState()`: Landmark proximity firing + the recheck cadence timestamp.
- L1283 `void ResetPendingPlatzState()`: Deferred "Platz" announcement.
- L1288 `void ResetGatedRefireState()`: Post-gate cluster re-announce (combat / blocking UI swallowed a change).
- L1293 `void Tick()`
- L1680 `bool IsWorldSpeechGated()`
- L1684 `void SpeakRegionLabel(const char* text)`
- L1688 `bool IsModuleLoadPending()`
- L1692 `void NotifyExternalLoadStarting(const char* reason)`
- L1712 `bool LooksLikeResref(const char* s)`: A destination module resref is short printable ASCII — "endar_spire", "tar_m02ac", "101PER".
- L1731 `bool TryReadExoString(void* base, char* out, size_t outSize)`: `base` is a CExoString* { char* c_string; uint32 length }.
- L1746 `static void AnnouncePreLoadDest(const char* dest, const char* shape)`: Shared tail of both hooks: dedup, then speak + log the destination.
- L1782 `void AnnouncePreLoadDestination(void* exoStringPtr)`
- L1817 `void LatchPreLoadDestinationK2(void* exoStringPtr)`
- L1854 `extern "C" void __cdecl OnSetMoveToModuleString(void* /*serverApp*/, void* arg_addr)`: CServerExoApp::SetMoveToModuleString — entry hook @0x004aecd0.
- L1880 `void __cdecl OnSetMoveToModuleStringK2(void* ebp)`
