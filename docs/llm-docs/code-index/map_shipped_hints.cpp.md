# map_shipped_hints.cpp (76 lines)

Source: `patches/Accessibility/map_shipped_hints.cpp`

(From map_shipped_hints.h.) Curated map hints shipped with the mod.

A small static table of (module resref, world position, localised label) for story spots the game never marks but blind players struggle to find: the Sandral estate backdoor, the Promised-Land rebel corpses in the Taris sewers, the bantha grazing band by the krayt-dragon cave. Positions come straight from the module .git data (verified against the K1CP-patched .mods 2026-07-27).

Speech/cycle-only: entries fold into the Map hint cycle listing (cycle_state) fog-of-war-gated through the engine's own IsWorldPointExplored — they reveal exactly like vanilla map notes — and are NEVER materialised as engine CSWCMapPins, so nothing is drawn on the visible map and no engine lifetime applies. Listing entries carry a pointer to the table row (static storage, stable forever); the activation path stamps them with the map-pin slot shape (frozen position), which narrated_target validates back through IsShippedHint.

## Declarations (in source order)

- L29 `bool ModuleMatches(const char* module)`
- L36 `const ShippedHint* AsTableRow(const void* p)`: Row-pointer identity: exact address of a table element.
- L45 `int CollectForCurrentModule(const ShippedHint** out, int maxOut)`
- L61 `bool IsShippedHint(const void* p)`
- L66 `bool GetLabel(const void* p, char* outBuf, size_t bufSize)`
