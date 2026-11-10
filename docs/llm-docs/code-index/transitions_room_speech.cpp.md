# transitions_room_speech.cpp (461 lines)

Source: `patches/Accessibility/transitions_room_speech.cpp`

What a region change says: the region label, the flap guard against a label spoken twice on a boundary, the delayed place line, and the refire of a change that was gated while speech was blocked.

## Declarations (in source order)

- L66 `bool IsFlapRepeat(const std::string& text, const Vector& pos, DWORD now)`: True when speaking `text` at `pos` would be the flap return-leg.
- L74 `void CommitSpokenLabel(const std::string& text, const Vector& pos, DWORD now)`: Rotate the two-deep spoken-label history.
- L97 `bool ResolveRoomSpeech(void* area, const Vector& worldPos, std::string& outBuf, const char*& outSource, int* outClusterIdOpt = nullptr, std::vector<acc::room_topology::ExitMark>* outExitsOpt = nullptr)`: Resolve the speech at `worldPos` using a two-tier order: 1.
- L167 `void LogWallTopoComparison(void* area, int roomIndex, const Vector& worldPos, const char* spoken, const char* source)`: Diagnostic — log room_topology's resolution at this position alongside whatever the resolved speech was (room name or shape).
- L193 `void SpeakPlayerRegionLabel(void* area, const std::string& label, const std::vector<acc::room_topology::ExitMark>& exits)`: The player's own region announce: exit_memory adds what lies behind each exit, whether it was visited, and which one leads back.
- L217 `void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos)`: Speak a perceptual-region change.
- L316 `void NoteGatedClusterChange()`: A cluster change was committed while speech was gated.
- L327 `void TickPendingPlaza(void* area, const Vector& playerPos)`: Fire a pending place line whose delay has elapsed.
- L400 `void TickGatedClusterRefire(void* area, const Vector& playerPos)`: Refire the current room label after a gated cluster change, once the gate (combat / blocking UI) has stayed clear for kGatedRefireDelayMs.
- L429 `bool IsResrefStyleRoomName(const char* name)`: Vanilla content names its rooms with the .lyt-room id (`m01aa_10`, `stunt_03_main`, `unk_m13ab`), meaningless through a screen reader, so such a name counts as none and the shape tier speaks instead; authored names ("...
- L450 `void ResetRoomSpeechState()`: Everything this file remembers about the area: the dedup key, the flap window, the pending place line and the gated refire.
