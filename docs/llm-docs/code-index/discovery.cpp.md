# discovery.cpp (328 lines)

Source: `patches/Accessibility/discovery.cpp`

(From discovery.h.) Discovery-driven cycling — the middle tier of object cycling.

Records which static/named objects the blind player has had narrated ORGANICALLY (passive ShowObject focus, Q/E line-of-sight, room-shape landmark callouts) and persists that index per-map IN THE SAVE, so the everyday in-world cycle (`,`/`.`) can resurface known objects without the spoilers/noise of the extended/map-side tier.

Storage primitive: named string vars on the player creature's CSWSScriptVarTable (engine_scriptvar.*; one var per area keyed by area tag). The index holds locale-independent KEYS only — never names; spoken names are regenerated live at cycle time.

Key model (stable across reload — positions/tags are fixed per area-load): * Named NPCs   — "N~<objTag>", eligible only when the tag is UNIQUE among creatures in the area (generic/duplicate mobs excluded). * Static (door / container / landmark / transition) — "S~<objTag>~<ord>", where <ord> is the deterministic ordinal among area objects sharing the same category + tag, in the order of narration::PositionLess. Tags repeat for static objects, so the ordinal disambiguates. * World items + generic enemies are OUT of scope (mobile / respawn / vanish) — DeriveKey returns ineligible, so they're never recorded or surfaced in the discovery tier.

Access: discovery is the DEFAULT filter for the in-world cycle. The existing "Extended cycling" mod setting, when ON, widens the same keys back to the full everything-on-map set (cycle_state consults the setting).

## Declarations (in source order)

- L51 `acc::filter::CycleCategory Classify(void* obj)`
- L61 `bool DeriveKey(void* obj, void* area, std::string& outKey)`: Derive the locale-independent per-area key for `obj`, or return false when the object is ineligible (item / non-nav / non-unique NPC / no tag).
- L119 `bool Contains(const std::string& key)`
- L128 `int MergeVar(const std::string& varName)`: Reads one save variable and appends its ';'-separated keys to g_keys, without duplicates and up to the cap.
- L147 `void LoadFromSave()`
- L156 `void Persist()`
- L201 `void ApplySeeds()`: Record every area object whose tag is on the current area's seed list.
- L232 `void OnAreaChanged(void* area)`
- L272 `void Tick()`
- L279 `void Record(void* gameObject)`
- L311 `bool IsDiscovered(void* gameObject)`
