# discovery.h (63 lines)

Source: `patches/Accessibility/discovery.h`

Discovery-driven cycling — the middle tier of object cycling.

Records which static/named objects the blind player has had narrated ORGANICALLY (passive ShowObject focus, Q/E line-of-sight, room-shape landmark callouts) and persists that index per-map IN THE SAVE, so the everyday in-world cycle (`,`/`.`) can resurface known objects without the spoilers/noise of the extended/map-side tier.

Storage primitive: named string vars on the player creature's CSWSScriptVarTable (engine_scriptvar.*; one var per area keyed by area tag). The index holds locale-independent KEYS only — never names; spoken names are regenerated live at cycle time.

Key model (stable across reload — positions/tags are fixed per area-load): * Named NPCs   — "N~<objTag>", eligible only when the tag is UNIQUE among creatures in the area (generic/duplicate mobs excluded). * Static (door / container / landmark / transition) — "S~<objTag>~<ord>", where <ord> is the deterministic ordinal among area objects sharing the same category + tag, in the order of narration::PositionLess. Tags repeat for static objects, so the ordinal disambiguates. * World items + generic enemies are OUT of scope (mobile / respawn / vanish) — DeriveKey returns ineligible, so they're never recorded or surfaced in the discovery tier.

Access: discovery is the DEFAULT filter for the in-world cycle. The existing "Extended cycling" mod setting, when ON, widens the same keys back to the full everything-on-map set (cycle_state consults the setting).

## Declarations (in source order)

- L39 `void OnAreaChanged(void* area)`: Area-change reconciliation hook.
- L44 `void Tick()`: Per-tick driver (called from core_tick).
- L55 `void Record(void* gameObject)`: Record `gameObject` as organically discovered.
- L60 `bool IsDiscovered(void* gameObject)`: True iff `gameObject` is in the current area's discovered set.
