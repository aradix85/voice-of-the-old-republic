# same_name_suffix.cpp (291 lines)

Source: `patches/Accessibility/same_name_suffix.cpp`

(From same_name_suffix.h.) Per-object disambiguator for same-LocName objects.

Multiple objects in an area often share a localized display name (5x "Wall Panel" in one corridor; 3x "Sith Soldier" in a fight). Sighted players tell them apart by position + reticle; blind players hear the same name in a row with no way to refer back.

Wraps engine::GetObjectName and appends a stable numeric suffix when two or more live objects in the current area share the resolved name. Suffix is keyed by server handle, assigned on first narration, and persists for the rest of the area lifetime — surviving members keep their slot even after others die.

## Declarations (in source order)

- L38 `int FindBucketIdx(const char* name)`
- L47 `int GetOrCreateBucketIdx(const char* name)`
- L57 `Entry* FindEntry(uint32_t handle)`
- L70 `void AppendEmptyContainerLabel(void* gameObject, char* outBuf, size_t bufSize)`: Live "empty" tag for loot containers.
- L83 `void AppendSuffix(void* gameObject, char* outBuf, size_t bufSize)`
- L144 `long RowBand(float y)`
- L150 `bool PositionLess(const Vector& a, const Vector& b)`
- L159 `int NumberingClass(void* gameObject)`
- L178 `void AppendAreaPositionOrdinal(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`
- L236 `void AppendDisambiguator(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`
- L250 `void AppendObjectQualifiers(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`
- L267 `bool GetSpokenName(void* gameObject, acc::filter::CycleCategory category, char* outBuf, size_t bufSize)`
- L280 `void Reset()`
