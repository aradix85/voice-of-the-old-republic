# menus_dispatch.cpp (330 lines)

Source: `patches/Accessibility/menus_dispatch.cpp`

GUI input dispatch and the focus-change hook. Two engine hooks:

* OnHandleFocusChange (CSWGuiControl::HandleFocusChange): log only. * OnHandleInputEvent (CSWGuiManager::HandleInputEvent): the ordered gate list every GUI key event runs through.

Gate ORDER is behaviour: each subsystem gets first refusal in a fixed sequence, so nothing here may be reordered without an in-game input pass.

## Declarations (in source order)

- L61 `bool IsPress(const Event& e)`
- L62 `bool IsEnter(int code)`
- L63 `bool IsEsc(int code)`
- L65 `void LogEvent(const Event& e, const char* tag)`
- L88 `int TrackPress(const Event& e, int rv)`: Every return of a press goes through here with its verdict (1 = consumed).
- L101 `bool IsReleaseOfConsumedPress(const Event& e)`
- L114 `bool IsModifierShadowed(const Event& e)`: The engine is modifier-blind: a mod hotkey that reuses an engine GUI key with a modifier would also drive the engine's bare-key handling.
- L133 `bool HelpOwns(const Event& e)`: A physical F1 reaches the manager as kInputActivate (0x27), the engine's GUI "activate" code, and would fire the focused control; help::PollWin32 owns F1.
- L159 `void* ResolveActivePanel()`: The panel the event is for: the manager's foreground (modal_stack or panels[]).
- L191 `void* ChainFocusOn(void* panel)`: The chain's focused control when the chain is on this panel.
- L203 `extern "C" void __cdecl OnHandleFocusChange(void* thisPtr, int param_1)`: CSWGuiControl::HandleFocusChange, hooked mid-function at 0x41896b.
- L224 `extern "C" int __cdecl OnHandleInputEvent(void* thisPtr, int param_1, int param_2)`: CSWGuiManager::HandleInputEvent, hooked mid-function at 0x0040c907: the GUI manager's central dispatcher, which every key or mouse event for any GUI surface passes before it goes to the active panel's own handler.
