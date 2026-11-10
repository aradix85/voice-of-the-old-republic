# menus_setactive.cpp (238 lines)

Source: `patches/Accessibility/menus_setactive.cpp`

The two SetActiveControl hooks, the engine's focus-change signals: the panel hook (CSWGuiPanel) feeds the pending-announce slot through menus_focus, the listbox hook (CSWGuiListBox) speaks the focused row. Both stay silent through the GUI rebuild burst of a cutscene or a load.

## Declarations (in source order)

- L41 `bool InMovieBurstWindow()`: The tail applies in-game only: at startup a movie ends right before the main menu, whose first-sight announce rides the SetActive the tail would swallow.
- L53 `const char* BurstSuppression()`: Why both hooks stay silent now, or nullptr.
- L61 `int ControlId(void* control)`
- L70 `void NoteMainMenuSetActive()`: The main menu's first SetActive is the engine's auto-focus on New Game; a second one comes from the engine's own mouse or keyboard focus path and so proves the input pump is live.
- L80 `void LogListBoxCursor(void* listBox, int rowCount)`: selection_index is -1 in scroll mode (bit_flags & 0x200), the row in selection mode.
- L93 `void LatchTabsCluster()`: The first listbox event after a panel change probes the focused panel for the Options layout (listbox at [0] plus a button cluster), whose tab buttons need the chain step's Y compensation.
- L106 `const char* SilencedRowReason(void* listBox, const char* text, int rowCount)`: Why a row with text stays silent, or nullptr.
- L135 `extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl)`: CSWGuiPanel::SetActiveControl, hooked mid-function at 0x0040a638 with EDI = the panel and ESI = the new active control (null when the panel drops its selection).
- L174 `extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow, int param2)`: CSWGuiListBox::SetActiveControl(CSWGuiControl* row, int param_2), hooked mid-function at 0x0041c16b, where all three arguments sit in registers: the 8-byte cut is PUSH EAX, PUSH ECX and LEA ECX,[ESI+0x29c], all positi...
