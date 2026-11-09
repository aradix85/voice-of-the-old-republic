# guidance_autowalk.h (85 lines)

Source: `patches/Accessibility/guidance_autowalk.h`

Autowalk — wrapper over CSWSCreature::AddMoveToPointAction.

Engine pathfinds + walks the character across walkmesh/transitions. We do not implement movement itself.

## Declarations (in source order)

- L19 `bool WalkTo(const Vector& destination)`: True = queued (does not guarantee arrival — pathfinder may fail, target may be unreachable, the player may interrupt).
- L25 `bool ForceWalkTo(const Vector& destination)`: Diagnostic alternate via CSWSCreature::ForceMoveToPoint — bypasses the per-creature queue but still pathfinds.
- L32 `bool UseObject(unsigned long targetHandle)`: Enqueue ACTION_USEOBJECT (0x28) — same primitive as NWScript's ActionInteractObject.
- L40 `bool CancelMovement()`: Clears the full action queue via CSWSObject::ClearAllActions — acceptable because the typical Shift+- stop case has only our move queued.
- L50 `void PollMovementKeysCancel()`: Cancel ANY mod-armed autowalk — Shift+- discovery (Cycle) or Enter interact (Interact) — while a W/S/A/D/C/Y movement key is held.

Also 4 constants or variables at namespace scope.
