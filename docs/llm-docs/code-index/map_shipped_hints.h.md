# map_shipped_hints.h (50 lines)

Source: `patches/Accessibility/map_shipped_hints.h`

Curated map hints shipped with the mod.

A small static table of (module resref, world position, localised label) for story spots the game never marks but blind players struggle to find: the Sandral estate backdoor, the Promised-Land rebel corpses in the Taris sewers, the bantha grazing band by the krayt-dragon cave. Positions come straight from the module .git data (verified against the K1CP-patched .mods 2026-07-27).

Speech/cycle-only: entries fold into the Map hint cycle listing (cycle_state) fog-of-war-gated through the engine's own IsWorldPointExplored — they reveal exactly like vanilla map notes — and are NEVER materialised as engine CSWCMapPins, so nothing is drawn on the visible map and no engine lifetime applies. Listing entries carry a pointer to the table row (static storage, stable forever); the activation path stamps them with the map-pin slot shape (frozen position), which narrated_target validates back through IsShippedHint.

## Declarations (in source order)

- L28 `struct ShippedHint`
- L37 `int CollectForCurrentModule(const ShippedHint** out, int maxOut)`: Fill `out` with up to `maxOut` pointers to this module's hints (stable table rows).
- L42 `bool IsShippedHint(const void* p)`: True when `p` is a row of the hint table AND its module matches the current module — the staleness contract narrated_target needs (a stamp from a previous area must invalidate on transition).
- L47 `bool GetLabel(const void* p, char* outBuf, size_t bufSize)`: Localised label for a table row previously handed out by CollectForCurrentModule / carried in a listing.
