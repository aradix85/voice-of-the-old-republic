# narrated_target.cpp (115 lines)

Source: `patches/Accessibility/narrated_target.cpp`

(From narrated_target.h.) Unified "last narrated target" slot.

Whichever narration channel spoke a bare-target-name most recently owns "what the user is thinking about". Activation keys (Enter, Shift+-, Ctrl+-, Alt+-, `-`) read this single slot instead of each resolving their own focus independently.

Stamp sites — passive_narrate (engine ShowObject), cycle_input (`,`/`.`/`-`), view_mode hover-pause. Non-name narration (pre-rolls, empty-state phrases, heading/turn/route lines) does NOT stamp.

Two slot shapes: game-object (uses server handle for validation) and map-pin (uses pointer membership in the client area's map_pins[]). Map pins have no CSWSObject so the activation grammar is reduced.

TryGet re-validates on every read and clears stale slots.

## Declarations (in source order)

- L18 `void Stamp(void* obj, uint32_t serverHandle)`
- L36 `void StampMapPin(void* pin, const Vector& pos)`
- L51 `void Clear()`
- L66 `bool IsMapPinStillPresent(void* pin)`: Quest scripts can call SetMapPinEnabled(off) — defensive membership walk.
- L81 `bool TryGet(Slot& out)`
