# menus_listbox_others.cpp (211 lines)

Source: `patches/Accessibility/menus_listbox_others.cpp`

Two small list screens: the examine view and the party AI script selection.

## Declarations (in source order)

- L68 `bool ExamineMatches(void* p)`
- L72 `void* ExamineFindLb(void* p)`
- L82 `void ExamineAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: The `rowCount <= 0` guard is this spec's own, and InGameMessagesAnnounce deliberately does not have it.
- L87 `bool ScriptSelectMatches(void* p)`
- L91 `void* ScriptSelectFindLb(void* p)`
- L99 `void ScriptSelectAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Speak the behaviour name + position, dropping the toggle's ", on"/", off" state suffix.
- L129 `void ScriptSelectEnrichRow(void* panel, const ListBoxNavResult& r)`: Per-row enrichment: speak the behaviour's description.
- L162 `bool ScriptSelectOnEnter(void* panel)`: Enter → Choose: applies the selected ai_state and closes.
- L168 `bool ScriptSelectOnEsc(void* panel)`
