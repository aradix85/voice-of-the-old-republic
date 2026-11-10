# engine_picker.h (150 lines)

Source: `patches/Accessibility/engine_picker.h`

Engine action picker — drive the game's context-sensitive action dispatcher for an arbitrary target.

Engine-only: sets up engine state, calls engine entry points, returns observation. No menu state, no speech.

Mechanism: when the cursor hovers an in-world object, the engine's DoPassiveSelection + GetDefaultActions pipeline computes the appropriate default action (open / talk / Security / Bash / Disable Trap / ...). A left-click dispatches via HandleMouseClickInWorld reading a descriptor at this+0x4c8.

Our cycle picks targets without moving the cursor, so +0x4c8 stays tied to the last hover. We replicate the cursor setup explicitly: 1. CGuiInGame.main_interface.field1_0x64 = targetClient 2. GetDefaultActions(this) — populates +0x4c8 / +0x4cc 3. Engine click gate (last_target = last_clicked_on_target = hover_target = X) 4. HandleMouseClickInWorld(this) — engine runs the picker

Engine surfaces: HandleMouseClickInWorld @0x00620350 GetDefaultActions       @0x00620620 SetMainInterfaceTarget  @0x0062b000 CSWGuiInterfaceAction layout: +0x00 CExoString label    ("Security", "Open", ...) +0x08 ulong     action_id (0x404 noop, 0x3ea talk, 0x3f7 use, 0x3f5 bash, 0x3f2 door toggle, 0x3f4 disable mine, ...) +0x0c void*     action_function +0x1c ulong     target_id (client-side, with 0x80000000) +0x20 CResRef   icon

## Declarations (in source order)

- L58 `inline bool IsWalkToActVerb(uint32_t action_id)`: Walk-to-act world verbs that GetDefaultActions emits for targets the PC must approach before acting: 0x3ea talk, 0x3f7 use/open, 0x3f5 bash, 0x3f2 door toggle, 0x3f4 disable mine.
- L72 `struct ActionSnapshot`: Snapshot of the first descriptor at +0x4c8 after GetDefaultActions.
- L110 `bool Drive(uint32_t targetServerHandle, ActionSnapshot* outSnapshot, bool forceRadial = false, bool populateOnly = false)`: targetServerHandle = server-side id (engine::GetObjectHandle).
- L124 `bool ReanchorRadial(uint32_t targetServerHandle)`: Re-assert the radial's target and rebuild the engine target-action menu for it, WITHOUT dispatching.
- L129 `bool ReadCurrent(ActionSnapshot* outSnapshot)`: Read +0x4c8 without driving anything — observe the engine's own picker (cursor-hover or passive-selection-driven) for diagnostics or refined narration of what Enter would pick now.
- L147 `bool InitiateDialog(uint32_t targetServerHandle)`: Start a conversation with the target DIRECTLY via CSWCCreature::Action- InitiateDialog @0x0060f620 — the function HandleMouseClickInWorld's confirm branch calls.

Also 1 constants or variables at namespace scope.
