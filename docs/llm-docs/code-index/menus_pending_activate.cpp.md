# menus_pending_activate.cpp (288 lines)

Source: `patches/Accessibility/menus_pending_activate.cpp`

Activating a control by hand (vtable HandleInputEvent), with the guards and repairs around it: a stale chain panel, a chargen sub-screen closing, a button that frees itself, and the journal and inventory lists the activation rebuilds.

## Declarations (in source order)

- L56 `void DrainActivate(const PendingOp& op)`: Direct activate via vtable[15].HandleInputEvent(0x27, 1).
