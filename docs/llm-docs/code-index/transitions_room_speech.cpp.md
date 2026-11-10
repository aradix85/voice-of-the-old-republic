# transitions_room_speech.cpp (485 lines)

Source: `patches/Accessibility/transitions_room_speech.cpp`

What a region change says: the region label, the flap guard against a label spoken twice on a boundary, the delayed place line, and the refire of a change that was gated while speech was blocked.

## Declarations (in source order)

- L44 `bool IsFlapRepeat(const std::string& text, const Vector& pos, DWORD now)`: True when speaking `text` at `pos` would be the flap return-leg.
- L52 `void CommitSpokenLabel(const std::string& text, const Vector& pos, DWORD now)`: Rotate the two-deep spoken-label history.
- L82 `bool ResolveRoomSpeech(void* area, const Vector& worldPos, std::string& outBuf, const char*& outSource, int* outClusterIdOpt = nullptr, std::vector<acc::room_topology::ExitMark>* outExitsOpt = nullptr)`: Resolve the speech at `worldPos` using a two-tier order: 1.
- L157 `void LogWallTopoComparison(void* area, int roomIndex, const Vector& worldPos, const char* spoken, const char* source)`: Diagnostic — log room_topology's resolution at this position alongside whatever the resolved speech was (room name, shape, or transition compose).
- L202 `void SpeakPlayerRegionLabel(void* area, const std::string& label, const std::vector<acc::room_topology::ExitMark>& exits)`: Speak a perceptual-region change.
- L211 `void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos)`
- L319 `void TickPendingPlatz(void* area, const Vector& playerPos)`: Fire any pending Platz announce whose delay has elapsed.
- L396 `void TickGatedClusterRefire(void* area, const Vector& playerPos)`: Refire the current room label after a gated cluster change, once the gate (combat / blocking UI) has stayed clear for kGatedRefireDelayMs.
- L419 `bool IsResrefStyleRoomName(const char* name)`
- L455 `void ResetRoomSpeechState()`: Cluster / room-label speech, including the text-equality dedup key and the flap-suppression window.
- L470 `void ResetPendingPlatzState()`: Deferred "Platz" announcement.
- L475 `void ResetGatedRefireState()`: Post-gate cluster re-announce (combat / blocking UI swallowed a change).
- L480 `void SpeakRegionLabel(const char* text)`
