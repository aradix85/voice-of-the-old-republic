# menus_listbox_skillinfo.cpp (267 lines)

Source: `patches/Accessibility/menus_listbox_skillinfo.cpp`

The skill and feat information box: its list, the feat behind a row, the row text with details, and its title.

## Declarations (in source order)

- L43 `bool SkillInfoBoxMatches(void* p)`
- L47 `void* SkillInfoBoxFindLb(void* p)`
- L55 `void* FindFeatsCharGenPanel()`: Walk the manager's panels[] for a CSWGuiFeatsCharGen instance.
- L73 `int ResolveFeatIdFromRowStrref(void* row, int& outRowStrref)`: Reverse-lookup: given the strref the engine wrote onto a SkillEntry row (kLabelStrRefOffset within the SkillEntry — its label_hilight.label.text .text_params.str_ref), find the matching feat in Rules->feats[] and retu...
- L102 `void SkillInfoBoxAnnounce(void* /*lb*/, const ListBoxNavResult& r)`: Standard per-row speech: just "feat name, N of M".
- L120 `void SkillInfoBoxEnrichRow(void* /*panel*/, const ListBoxNavResult& r)`: Per-row enrichment: refresh the underlying CSWGuiFeatsCharGen's description_listbox via OnEnterFeat(featId), then read + speak that description.
- L186 `bool SkillInfoBoxOnEnter(void* panel)`
- L202 `bool SkillInfoBoxTitleIsPlaceholder(void* panel, char* outText, size_t outSize)`: Read the panel's own title label, and say whether it still holds the BioWare dev-leftover baked into skillinfo.gui ("Items Available to Place in Container and blah blah blah").
- L218 `const char* SkillInfoBoxTitleOverride(void* panel)`: Title override: substitute our "You receive these feats" string for the BioWare placeholder text baked into skillinfo.gui.
