# menus_equipstats.h (59 lines)

Source: `patches/Accessibility/menus_equipstats.h`

virtual stat-row anchors for the Equip panel.

Sighted players see four computed stat values rendered at the bottom of the equipment screen (CSWGuiInGameEquip): Vitality (HP), Defense (AC), Attack to-hit, and Damage range. Each is drawn into an inline CSWGuiLabel inside the panel struct. The labels aren't IsChainNavigable, so the chain walker would skip them — we surface them as virtual text-only chain entries the same way menus_credits does for the credits readout and menus_charsheet does for the Charakterblatt attribute block.

Ordering: stats anchor at sortCy values higher than every real button on the panel so they read AFTER the 9 slot buttons and the Back / Change-character buttons.

Read path: inline CSWGuiLabel.gui_string holds the engine's actually- rendered text. Attack splits into single-weapon (LBL_TOHIT) vs dual- wield (LBL_TOHITL + LBL_TOHITR) at render time; we anchor on tohit_label and choose the format string by whether the left/right tohit labels carry text.

Wired by: * menus_chain.cpp — RebindChain: ForEachEquipStatRowAnchor registers a text-only chain entry per stat value. * menus_extract.cpp — FromControl section 0: IsEquipStatRowAnchor + ExtractEquipStatRow override the bare label text with the localised composed phrase.

## Declarations (in source order)

- L37 `bool IsEquipStatRowAnchor(void* panel, void* labelControl)`: True iff `labelControl` is one of the inline stat-value labels for an InGameEquip `panel`.
- L45 `void ForEachEquipStatRowAnchor(void* panel, bool (*callback)(void* labelControl, int sortCy, void* userData), void* userData)`: Iterate the equip-stat-row anchors for `panel`.
- L55 `bool ExtractEquipStatRow(void* panel, void* labelControl, char* outBuf, size_t bufSize)`: Read the stat label's rendered text and format the localised line.
