# stealth_watch.cpp (149 lines)

Source: `patches/Accessibility/stealth_watch.cpp`

(From stealth_watch.h.) Stealth distance readout.

While the active leader has Stealth mode engaged AND a hostile creature is the current narrated-target focus, speak the 2-D distance (bare number, in metres) each time it changes by at least kStepMeters as the gap closes or widens. Purpose: a blind player sneaking toward an enemy hears the closing distance, so they know when they are inside Sneak Attack range (< 10 m, where a strike from stealth lands the passive bonus damage).

Silent in all normal play — the module is inert unless the leader is actually stealthed AND a hostile creature is focused. Reads only (leader stealth_mode byte, focus distance); no hooks, driven from the per-tick dispatcher.

## Declarations (in source order)

- L45 `bool ReadLeaderStealth(void* leader)`
- L55 `void ResetBaseline()`
- L68 `int Distance2DMeters(void* target)`: Rounded 2-D (floor-plane) distance in metres between the leader and target, matching the examine panel's distance readout.
- L79 `void Tick()`
