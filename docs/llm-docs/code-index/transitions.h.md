# transitions.h (99 lines)

Source: `patches/Accessibility/transitions.h`

Pillar 2 — area + room transition announcements.

Poll-based. Each Tick: compare area pointer + room index against last observed; speak on delta. First observation after player-load also speaks (gives orientation on game-load / DLL-injection).

Pre-load destination announce ("Loading: <module>") goes through the OnSetMoveToModuleString detour (engine entry @0x004aecd0).

Speech uses interrupt=false — transitions don't talk over in-flight cycle / interact / passive_narrate.

## Declarations (in source order)

- L22 `void Tick()`
- L27 `void AnnouncePreLoadDestination(void* exoStringPtr)`: exoStringPtr = engine's CExoString* (destination resref).
- L32 `void LatchPreLoadDestinationK2(void* exoStringPtr)`: KOTOR 2 twin.
- L40 `bool FindLandmarkNear(const Vector& pos, float rangeM, char* nameOut, size_t nameBufSize, Vector& posOut, int* outLandmarkIdx = nullptr)`: Closest landmark waypoint to pos within rangeM.
- L53 `bool IterateLandmarks(void* area, int& cursor, char* nameOut, size_t nameBufSize, Vector& posOut, int& outLandmarkIdx)`: Walk the cache.
- L60 `void MarkLandmarkClaimedByDoor(int landmarkIdx)`: room_topology embedded this landmark into a cluster label — suppress the per-tick proximity announce so the same name doesn't fire twice within a second.
- L65 `bool IsResrefStyleRoomName(const char* name)`: Vanilla CSWSArea.room_names[] entries are .lyt-room ids ("m01aa_10", "stunt_03_main"); mods occasionally supply friendly strings.
- L71 `bool IsWorldSpeechGated()`: True iff in-world ambient announces (room transitions, region cursor) should stay silent this tick.
- L77 `void SpeakRegionLabel(const char* text, const std::vector<acc::room_topology::ExitMark>& exits)`: Speaks a perceptual-region label (the walking announce and the view-mode cursor's): through 3D exit descriptions when they are on, each of `exits` sounding from where it leads, else the announcement voice, centred.
- L87 `bool IsModuleLoadPending()`: True between SetMoveToModuleString firing and the next fresh area pointer surfacing.
- L96 `void NotifyExternalLoadStarting(const char* reason)`: Arm the module-load latch for a load path the OnSetMoveToModuleString detour doesn't see — specifically a save-game LOAD launched from the menu (the engine reconstructs the whole in-game GUI without routing through Se...
