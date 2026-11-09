# map_user_markers.cpp (200 lines)

Source: `patches/Accessibility/map_user_markers.cpp`

(From map_user_markers.h.) Saved user markers on the area map.

Shift+N drops a CSWCMapPin at the map cursor position. Auto-named from the cursor's room/landmark + per-area sequence number. Folds into the MapPin cycle category immediately.

Persists for the current area-load only. Engine-side persistence uses per-player CSWSScriptVarTable variables; replicating that is follow-up work. Marker vanishes on area transition or save+reload.

kUserMarkerReferenceBase is an arbitrary value handed to the engine as the pin's reference-number; it is NOT a reliable "this is ours" signal. Engine map-note pins are keyed by the waypoint's client object id, which always carries the 0x80000000 high bit, so a reference-number range test can't distinguish our markers from the engine's. Identity tracking (IsUserMarkerPin) is the authoritative discriminator instead.

## Declarations (in source order)

- L36 `void MaybeResetForArea(void* currentArea)`: Reset the per-area sequence + marker registry when the player changes area.
- L48 `void RegisterUserMarker(void* pin)`: Record a pin we just created so IsUserMarkerPin can recognise it.
- L64 `void BuildAutoName(void* area, const Vector& pos, int seq, char* outBuf, size_t bufSize)`: Compose a friendly auto-name from the cursor's spot.
- L105 `void OnDrop()`
- L174 `bool IsUserMarkerPin(void* pin)`
- L188 `void PollWin32()`
