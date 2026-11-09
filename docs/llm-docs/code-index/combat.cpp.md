# combat.cpp (313 lines)

Source: `patches/Accessibility/combat.cpp`

when combat ends (experimental) transient (engine LYT loader use-after-free)

## Declarations (in source order)

- L40 `bool ReadCombatMode(int& outMode)`
- L56 `bool IsCombatActive()`
- L66 `bool IsPartyInCombat()`: Encounter-level combat: OR of the controlled leader's global combat bit and every party member's per-creature bit.
- L72 `void TickCombatMode()`
- L201 `void* FindInGameMessagesPanel()`: Resolve the persistent combat-log panel via the CGuiInGame singleton.
- L214 `int ReadListBoxRowCount(void* lb)`: Read a CSWGuiListBox's row count without dereferencing each row pointer.
- L227 `void* ReadListBoxRow(void* lb, int i)`: Read row[i] as a control pointer.
- L242 `void TickCombatLog()`
