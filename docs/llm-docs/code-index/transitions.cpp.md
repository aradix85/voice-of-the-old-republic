# transitions.cpp (1919 lines)

Source: `patches/Accessibility/transitions.cpp`

region labels (nav-graph decomposition)

## Declarations (in source order)

- L194 `void TrackMotion(const Vector& pos)`
- L216 `void ResetMotion()`
- L221 `float CentreDistance(const Vector& pos)`
- L227 `bool CentreRuleReady(int clusterId, const Vector& pos)`: True once the pending centre-rule cluster may speak.
- L343 `void SpeakArea(void* area)`
- L374 `void RebuildLandmarkCache(void* area)`
- L494 `bool IsFlapRepeat(const std::string& text, const Vector& pos, DWORD now)`: True when speaking `text` at `pos` would be the flap return-leg.
- L502 `void CommitSpokenLabel(const std::string& text, const Vector& pos, DWORD now)`: Rotate the two-deep spoken-label history.
- L576 `void RecomputeLandmarkRanges()`
- L650 `bool ResolveRoomSpeech(void* area, const Vector& worldPos, std::string& outBuf, const char*& outSource, int* outClusterIdOpt = nullptr, std::vector<acc::room_topology::ExitMark>* outExitsOpt = nullptr)`: Resolve the speech at `worldPos` using a two-tier order: 1.
- L725 `void LogWallTopoComparison(void* area, int roomIndex, const Vector& worldPos, const char* spoken, const char* source)`: Diagnostic — log room_topology's resolution at this position alongside whatever the resolved speech was (room name, shape, or transition compose).
- L770 `void SpeakPlayerRegionLabel(void* area, const std::string& label, const std::vector<acc::room_topology::ExitMark>& exits)`: Speak a perceptual-region change.
- L782 `void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos)`
- L892 `void TickPendingPlatz(void* area, const Vector& playerPos)`: Fire any pending Platz announce whose delay has elapsed.
- L972 `void TickGatedClusterRefire(void* area, const Vector& playerPos)`: Refire the current room label after a gated cluster change, once the gate (combat / blocking UI) has stayed clear for kGatedRefireDelayMs.
- L1007 `bool TryAutoDiscoverMapNote(void* waypoint, const Vector& playerPos)`: If `waypoint` (a landmark whose note is still disabled) sits within auto-discover range of the leader, enable it via the engine's own field write.
- L1035 `void TickLandmarkCacheRecheck(void* area, const Vector& playerPos)`: Detect map notes whose enabled flag flipped since the last cache scan and rebuild.
- L1079 `void TickProximityLandmarks(const Vector& playerPos)`: Per-tick scan over the landmark cache.
- L1169 `bool IsWorldSpeechGatedImpl()`: Forward declaration of the public predicate (defined at file scope below namespace).
- L1178 `bool IsResrefStyleRoomName(const char* name)`
- L1200 `bool FindLandmarkNear(const Vector& pos, float rangeM, char* nameOut, size_t nameBufSize, Vector& posOut, int* outLandmarkIdx)`
- L1228 `bool IterateLandmarks(void* area, int& cursor, char* nameOut, size_t nameBufSize, Vector& posOut, int& outLandmarkIdx)`
- L1258 `void MarkLandmarkClaimedByDoor(int landmarkIdx)`
- L1277 `void ResetRoomSpeechState()`: Cluster / room-label speech, including the text-equality dedup key and the flap-suppression window.
- L1292 `void ResetLandmarkProximityState()`: Landmark proximity firing + the recheck cadence timestamp.
- L1300 `void ResetPendingPlatzState()`: Deferred "Platz" announcement.
- L1305 `void ResetGatedRefireState()`: Post-gate cluster re-announce (combat / blocking UI swallowed a change).
- L1310 `void Tick()`
- L1700 `bool IsWorldSpeechGated()`
- L1704 `void SpeakRegionLabel(const char* text, const std::vector<acc::room_topology::ExitMark>& exits)`
- L1711 `bool IsModuleLoadPending()`
- L1715 `void NotifyExternalLoadStarting(const char* reason)`
- L1735 `bool LooksLikeResref(const char* s)`: A destination module resref is short printable ASCII — "endar_spire", "tar_m02ac", "101PER".
- L1754 `bool TryReadExoString(void* base, char* out, size_t outSize)`: `base` is a CExoString* { char* c_string; uint32 length }.
- L1769 `static void AnnouncePreLoadDest(const char* dest, const char* shape)`: Shared tail of both hooks: dedup, then speak + log the destination.
- L1806 `void AnnouncePreLoadDestination(void* exoStringPtr)`
- L1841 `void LatchPreLoadDestinationK2(void* exoStringPtr)`
- L1878 `extern "C" void __cdecl OnSetMoveToModuleString(void* /*serverApp*/, void* arg_addr)`: CServerExoApp::SetMoveToModuleString — entry hook @0x004aecd0.
- L1904 `void __cdecl OnSetMoveToModuleStringK2(void* ebp)`
