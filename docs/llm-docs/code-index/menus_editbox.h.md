# menus_editbox.h (78 lines)

Source: `patches/Accessibility/menus_editbox.h`

editbox (text-input field) input dispatcher + monitor.

Two vanilla users: the chargen Name screen's `name_editbox` and the save- name popup's `edit_box` (CSWGuiSaveGameEditBox). The TU is shaped as a spec-table mirror of menus_listbox.{h,cpp} so adding another editbox is one new spec value, not another copy of the dispatch+monitor scaffolding.

Edit-mode model (patch-internal, NOT an engine state):

* Auto-enter when chain focus lands on the editbox. The focus-enter speech ("Eingabefeld. <text>") is produced by FromControl in menus_extract_controls.cpp (TryEditbox) — same pattern as slider / cycle / label extractions. The per-tick monitor here only initialises the diff snapshot on this edge; it does not speak. * In edit mode, this dispatcher consumes: - Up / Down → re-speak the full current text (single-line field, so vertical motion has nothing else to do). - Enter     → fire the panel's submit button (BTN_OK / "Accept") via the deferred-op queue and exit edit mode. No confirmation speech — the next panel's own announce takes over. - Esc       → silently exit edit mode. The editbox keeps engine focus; subsequent Up/Down then fall through to chain nav so the user can move to OK / Random / Cancel. * Letter / Backspace / Left / Right are NOT consumed — the engine handles caret movement and text mutation natively. The per-tick poll diffs the (text, caret) snapshot each frame and announces: - length grew by 1 → speak the inserted char (caret-1) - length shrunk by 1 → speak the deleted char (from prior snapshot) - |Δlength| > 1 → speak the full new text (covers Zufallsname button replacing the field in one shot) - caret moved without length change → speak char at new caret; "end" if the caret is past the last char.

Why focus-enter speech lives in extract (not here): keeps the per-control announce path uniform — chain nav's AnnounceControl always asks FromControl, and any control-class with composed speech (slider, cycle widgets, this editbox) plugs in there. The monitor is then a thin "diffs after the entry announce" layer.

## Declarations (in source order)

- L55 `bool TryHandleInput(int n, void* thisPtr, void* activePanel, int param_1, int param_2, int& outRv)`: Try to handle the input event against the active editbox spec.
- L63 `void TickEditboxMonitors()`: Per-tick monitor.
- L75 `const char* GetTitleOverride(void* panel)`: Title-speech override lookup.
