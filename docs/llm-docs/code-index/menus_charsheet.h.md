# menus_charsheet.h (68 lines)

Source: `patches/Accessibility/menus_charsheet.h`

character sheet stat rows.

CSWGuiInGameCharacter has 17+ inline labels (class / level / HP / FP / XP / 6 attributes + their pre-formatted modifiers / alignment slider). None of them is IsChainNavigable, so this module surfaces them as virtual, text-only chain entries the user arrows through — each read as a composed localised phrase ("Strength 14, +2").

Opening the panel speaks its name only ("Character"), from the generic sub-screen announce in menus_monitors.cpp. There is deliberately NO read-everything-on-open path: an earlier opener that composed all 12 rows into one line was removed once the rows became arrow-reachable, and the generic content-fingerprint monitor is silent on this kind for the same reason (it spoke the value labels as bare context-free numbers — "14", "120000"). The fingerprint is still TRACKED for this kind, because a content change is what rebinds the chain when Tab swaps the displayed party member and the Force-points row appears or disappears.

## Declarations (in source order)

- L45 `bool ExtractStatRow(void* panel, void* labelControl, char* outBuf, size_t bufSize)`: Per-row formatted speech for one character sheet value label.
- L52 `bool IsStatRowAnchor(void* panel, void* labelControl)`: True iff `labelControl` is a known stat-row anchor in `panel`.
- L62 `void ForEachStatRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`: Iterate all stat-row anchor pointers for `panel`.
