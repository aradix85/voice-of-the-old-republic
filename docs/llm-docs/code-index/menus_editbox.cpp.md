# menus_editbox.cpp (651 lines)

Source: `patches/Accessibility/menus_editbox.cpp`

editbox dispatcher + per-tick monitor.

See menus_editbox.h for the model (soft edit-mode flag, spec-table over panel kinds, focus-enter speech via FromControl, diff polling here).

Single armed editbox at a time — vanilla KOTOR has exactly one editbox in the entire game (chargen Name's `name_editbox`); the file-static `s_state` reflects that with one spec/panel/editbox triple. If a future mod-introduced second editbox ever appears in the wild and the user can have both panels active simultaneously, this becomes a small array.

## Declarations (in source order)

- L81 `bool ChargenNameMatches(void* panel)`
- L87 `void* ChargenNameFindEditbox(void* panel)`
- L92 `void* ChargenNameFindSubmitButton(void* panel)`
- L102 `const char* ChargenNameTitleOverride(void* panel)`: Substitute the chargen Name panel's title speech: read subtitle_label ("Name") instead of letting the generic walk land on main_title_label ("CHARAKTERAUSWAHL").
- L125 `bool SaveNameMatches(void* panel)`
- L131 `void* SaveNameFindEditbox(void* panel)`
- L136 `void* SaveNameFindSubmitButton(void* panel)`
- L144 `const char* SaveNameTitleOverride(void* panel)`: Substitute the save-name popup's title speech with its own title_label.
- L232 `bool ReadEditbox(void* editbox, char* outText, size_t outCap, uint32_t& outLen, uint32_t& outRawLen, short& outA, short& outB)`: Read (text, length, shortA, shortB, rawLen) from the editbox into `out`.
- L255 `void SnapshotInto(ArmedState& s)`
- L264 `void SpeakFullText(const char* text, uint32_t len)`
- L275 `void SpeakSingleChar(char c)`: Speak a single character as a one-char string; NVDA reads single chars per its "characters" verbosity setting.
- L284 `void PollAndAnnounceDiff(ArmedState& s)`
- L353 `void PollModalKeys(ArmedState& s)`
- L434 `PanelMatch FindMatchingPanel()`
- L464 `void DisarmIfArmed(const char* reason)`
- L477 `bool TryHandleInput(int /*n*/, void* /*thisPtr*/, void* activePanel, int param_1, int param_2, int& outRv)`
- L545 `void TickEditboxMonitors()`
- L639 `const char* GetTitleOverride(void* panel)`
