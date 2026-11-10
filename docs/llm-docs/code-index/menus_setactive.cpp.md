# menus_setactive.cpp (345 lines)

Source: `patches/Accessibility/menus_setactive.cpp`

The two SetActiveControl hooks, the engine's focus-change signals: the panel hook (CSWGuiPanel) feeds the pending-announce slot through menus_focus, the listbox hook (CSWGuiListBox) speaks the focused row. Both stay silent through the GUI rebuild burst of a cutscene or a load.

## Declarations (in source order)

- L62 `bool InMovieBurstWindow()`: True while the engine-GUI SetActive rebuild burst must stay silent because a cutscene movie is on screen — or was, within the last kMovieBurstTailMs.
- L83 `const char* BurstSuppression()`: Why both SetActiveControl hooks stay silent right now, or nullptr.
- L91 `int ListBoxRowCount(void* listBox)`
- L111 `extern "C" void __cdecl OnSetActiveControl(void* panel, void* newControl)`: CSWGuiPanel::SetActiveControl — hooked mid-function at 0x0040a638.
- L193 `extern "C" void __cdecl OnListBoxSetActiveControl(void* listBox, void* newRow, int param2)`: CSWGuiListBox::SetActiveControl — hooked mid-function at 0x0041c16b.
