# menus_listbox_others.cpp (203 lines)

Source: `patches/Accessibility/menus_listbox_others.cpp`

Two small list screens: the examine view and the party AI script selection.

## Declarations (in source order)

- L60 `bool ExamineMatches(void* p)`
- L64 `void* ExamineFindLb(void* p)`
- L74 `void ExamineAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: The `rowCount <= 0` guard is this spec's own, and InGameMessagesAnnounce deliberately does not have it.
- L79 `bool ScriptSelectMatches(void* p)`
- L83 `void* ScriptSelectFindLb(void* p)`
- L91 `void ScriptSelectAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak the behaviour name + position, dropping the toggle's ", ein"/", aus" state suffix.
- L121 `void ScriptSelectEnrichRow(void* panel, const ListBoxNavResult& r)`: Per-row enrichment: speak the behaviour's description.
- L154 `bool ScriptSelectOnEnter(void* panel)`: Enter → Wählen: applies the selected ai_state and closes.
- L160 `bool ScriptSelectOnEsc(void* panel)`
