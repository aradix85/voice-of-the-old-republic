# discovery.cpp (397 lines)

Source: `patches/Accessibility/discovery.cpp`

(From discovery.h.) Discovery-driven cycling — the middle tier of object cycling.

Records which static/named objects the blind player has had narrated ORGANICALLY (passive ShowObject focus, Q/E line-of-sight, room-shape landmark callouts) and persists that index per-map IN THE SAVE, so the everyday in-world cycle (`,`/`.`) can resurface known objects without the spoilers/noise of the extended/map-side tier.

Storage primitive: named string vars on the player creature's CSWSScriptVarTable (engine_scriptvar.*; one var per area keyed by area tag). The index holds locale-independent KEYS only — never names; spoken names are regenerated live at cycle time.

Key model (stable across reload — positions/tags are fixed per area-load): * Named NPCs   — "N~<objTag>", eligible only when the tag is UNIQUE among creatures in the area (generic/duplicate mobs excluded). * Static (door / container / landmark / transition) — "S~<objTag>~<ord>", where <ord> is the deterministic north-to-south ordinal among area objects sharing the same category + tag (mirrors cycle_input's PositionLess). Tags repeat for static objects, so the ordinal disambiguates. * World items + generic enemies are OUT of scope (mobile / respawn / vanish) — DeriveKey returns ineligible, so they're never recorded or surfaced in the discovery tier.

Access: discovery is the DEFAULT filter for the in-world cycle. The existing "Extended cycling" mod setting, when ON, widens the same keys back to the full everything-on-map set (cycle_state consults the setting).

## Declarations (in source order)

- L57 `bool PositionLess(const Vector& a, const Vector& b)`: North-to-south total order — mirrors cycle_input::PositionLess so the discovery ordinal matches the spoken "Nordpfad 3" numbering the player already hears.
- L72 `acc::filter::CycleCategory Classify(void* obj)`
- L82 `bool DeriveKey(void* obj, void* area, std::string& outKey)`: Derive the locale-independent per-area key for `obj`, or return false when the object is ineligible (item / non-nav / non-unique NPC / no tag).
- L138 `bool Contains(const std::string& key)`
- L151 `int MergeVar(const char* varName)`: Read one save var and append its ';'-separated keys to g_keys, skipping duplicates and honouring the cap.
- L174 `void LoadFromSave()`
- L207 `void Persist()`
- L252 `void ApplySeeds()`: Record every area object whose tag is on the current area's seed list.
- L283 `void OnAreaChanged(void* area)`
- L329 `void Tick()`
- L348 `void Record(void* gameObject)`
- L380 `bool IsDiscovered(void* gameObject)`
