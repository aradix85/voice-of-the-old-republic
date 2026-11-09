# same_name_suffix.h (92 lines)

Source: `patches/Accessibility/same_name_suffix.h`

Per-object disambiguator for same-LocName objects.

Multiple objects in an area often share a localized display name (5x "Wandverkleidung" in one corridor; 3x "Sith-Soldat" in a fight). Sighted players tell them apart by position + reticle; blind players hear the same name in a row with no way to refer back.

Wraps engine::GetObjectName and appends a stable numeric suffix when two or more live objects in the current area share the resolved name. Suffix is keyed by server handle, assigned on first narration, and persists for the rest of the area lifetime — surviving members keep their slot even after others die.

## Declarations (in source order)

- L26 `bool GetSpokenName(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`: Mirrors engine::GetObjectName's contract: true on non-empty resolved name; false leaves outBuf empty.
- L36 `void AppendObjectQualifiers(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`: Append every qualifier that follows an object's base name, in the order they must be spoken: disambiguating number, engine state suffix, state override label, then the empty-loot note.
- L48 `void AppendDisambiguator(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`: Append the disambiguating number to an already-resolved name in outBuf, using the unified policy keyed on category: - Npc (mobile "mobs"): handle-keyed encounter serial (AppendSuffix) — creatures move, so a spatial ra...
- L56 `void AppendSuffix(void* gameObject, char* outBuf, size_t bufSize)`: Append a handle-keyed encounter serial to an already-resolved name in outBuf (combat narration paths use the engine's universal GetObjectDisplayNameByHandle).
- L77 `void AppendAreaPositionOrdinal(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`: Append a GLOBAL north-to-south positional ordinal to an already-resolved BASE name in outBuf.
- L86 `bool PositionLess(const Vector& a, const Vector& b)`: North-to-south world ordering.
- L89 `void Reset()`: Hooked into transitions.cpp's area-reset chain.
