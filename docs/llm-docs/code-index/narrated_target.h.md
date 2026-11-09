# narrated_target.h (49 lines)

Source: `patches/Accessibility/narrated_target.h`

Unified "last narrated target" slot.

Whichever narration channel spoke a bare-target-name most recently owns "what the user is thinking about". Activation keys (Enter, Shift+-, Ctrl+-, Alt+-, `-`) read this single slot instead of each resolving their own focus independently.

Stamp sites — passive_narrate (engine ShowObject), cycle_input (`,`/`.`/`-`), view_mode hover-pause. Non-name narration (pre-rolls, empty-state phrases, heading/turn/beacon waypoints) does NOT stamp.

Two slot shapes: game-object (uses server handle for validation) and map-pin (uses pointer membership in the client area's map_pins[]). Map pins have no CSWSObject so the activation grammar is reduced.

TryGet re-validates on every read and clears stale slots.

## Declarations (in source order)

- L26 `struct Slot`
- L36 `void Stamp(void* obj, uint32_t serverHandle)`: Both args must be server-side.
- L39 `void StampMapPin(void* pin, const Vector& pos)`: Map-pin stamp.
- L41 `void Clear()`
- L46 `bool TryGet(Slot& out)`: Validated read.
