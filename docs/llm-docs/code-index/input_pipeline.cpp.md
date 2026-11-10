# input_pipeline.cpp (395 lines)

Source: `patches/Accessibility/input_pipeline.cpp`

The engine's in-world input route, CClientExoAppInternal::HandleInputEvent (both games): which presses the mod takes before the engine sees them, the preparation of the bare action keys 1..7, and the frame sequence counter.

## Declarations (in source order)

- L49 `bool ConsumeLatch(DWORD& at, DWORD window)`
- L58 `unsigned int NextSeq()`
- L62 `void NoteOverlayEscClosed()`
- L66 `bool ConsumeOverlayEscLatch()`
- L70 `void NoteEditboxSubmitClosed()`
- L74 `bool ConsumeEditboxSubmitLatch()`
- L92 `bool ReadEvent(void* p1_addr, void* p2_addr, InputEvent* ev)`: The two event args sit in the engine's frame at the addresses the hook passes (LEA of the slots); the return address is one slot below.
- L103 `void LogEvent(const InputEvent& ev, void* this_ptr)`
- L118 `bool IsGuiNavForward(int code)`
- L125 `int TargetRowForCode(int code)`: The engine's logical codes for bare 1..7: 1..3 are DoTargetAction rows 0..2; 4..7 are DoPersonalAction columns 0..3, linear in the key number, though the codes of keys 6 and 7 are swapped (6 = 0xee, 7 = 0xec).
- L134 `int PersonalColumnForCode(int code)`
- L144 `bool IsBareActionCode(int code)`
- L148 `const char* BareLabel(int code)`
- L162 `void PrepareBareDispatchForNarratedTarget()`: SetTarget + RePopulate of the engine's action lists against the narrated target (kInvalidObjectId when none resolves: empty target rows rather than stale ones).
- L177 `void RestampPersonalColumn(int col)`: PopulateMenus hands out fresh action ids, so the user's chosen variant is stamped again after the repopulate (DoPersonalAction falls back to variant 0 when the selected id matches nothing).
- L199 `void RestampTargetRow(int row)`: The same for the target rows: DoTargetAction searches the row's list for the stamped id, falling back to the first entry.
- L209 `bool TrySwoopJump(const InputEvent& ev)`: KOTOR 2 swoop race: Space (event 0xd9) is the engine's jump, and the mod's runner takes it so every jump is logged beside the race state.
- L220 `bool TryPuzzleBareR(const InputEvent& ev)`: In the Rakatan floor-plate room bare R reads the board (floor_puzzle), so the engine's DefaultAction on the same press is swallowed.
- L237 `bool TryModifierReservation(const InputEvent& ev)`: The engine reads modifier-blind scancodes and has no combos of its own: Shift+4 still fires bare 4.
- L262 `bool TryOverlayEsc(const InputEvent& ev)`: The mod's in-world overlays (action menus, queue, examine, help) have no engine panel, so Esc lands here, where the engine would open the pause menu while the overlay's own poll closes it.
- L282 `void PrepareBareActionKey(const InputEvent& ev)`: Bare 1..7: the engine rebuilds its action lists only on a combat-mode flip, a sub-screen close or mouse hover, not on a Q/E target change, so between rounds they hold a stale creature_id (grenades at friends).
- L307 `bool TryQE(const InputEvent& ev)`: Q (205) / E (204).
- L323 `void ForwardModalArrows(const InputEvent& ev)`: A MessageBox goes on the modal stack without changing sw_gui_status, so the arrow codes are routed here, where nothing handles them, instead of to the manager: the user could not move between the popup's buttons.
- L344 `int HandleClientInputEvent(void* this_ptr, void* p1_addr, void* p2_addr)`: 1 consumes the event (the hook's consumed exit), 0 lets the engine run.
- L374 `extern "C" void __cdecl OnProcessInput(void* /*this_ptr*/)`: CClientExoAppInternal::ProcessInput (K1 hook 0x006227fb, K2 0x007B0EAB): the frame boundary, kept as a gap in the sequence of the input logs.
- L382 `extern "C" int __cdecl OnClientHandleInputEvent(void* this_ptr, void* p1_addr, void* p2_addr)`: CClientExoAppInternal::HandleInputEvent, hooked at its entry (K1 0x00621210): every event ProcessInput sends the client-app way (in-world hotkeys, pause, party cycle, target actions, Esc).
- L390 `extern "C" int __cdecl OnClientHandleInputEventK2(void* this_ptr, void* frame_ebp)`: KOTOR 2 (0x007B12C0, hooked at 0x007B12EE): ecx = this, ebp = the engine's frame with the args at [ebp+8]/[ebp+0xc] and the return address at [ebp+4], the same slot layout as K1, so the shared handler runs unchanged.
