# view_mode_narrate.cpp (214 lines)

Source: `patches/Accessibility/view_mode_narrate.cpp`

What view mode says about the cursor: the region it enters and the nearest object under it.

## Declarations (in source order)

- L25 `bool ResolveCursorRegionLabel(void* area, const Vector& cursor, std::string& outBuf, const char*& outSource)`: Tier order matches the walking adapter: landmark → friendly room name → shape cache.
- L81 `void AnnounceCursorRegion(void* area, const Vector& cursor)`: Per-tick region announce.
- L130 `void NarrateNearestObject(void* area, const Vector& cursor)`
