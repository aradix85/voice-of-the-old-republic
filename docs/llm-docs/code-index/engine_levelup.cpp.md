# engine_levelup.cpp (395 lines)

Source: `patches/Accessibility/engine_levelup.cpp`

the action menu, WITHOUT opening a sub-screen

## Declarations (in source order)

- L105 `void* GetInGameCharacterPanel(void* gui)`: Read the live CSWGuiInGameCharacter pointer from the CGuiInGame singleton.
- L120 `bool SetLevelUpMode(void* gui, int mode)`: Set CGuiInGame.level_up_mode via the engine's own setter.
- L156 `void DriveSWGuiStatus(void* gui, int status, int p2)`
- L203 `bool GrantedFeatsPopupStillModal()`: True while the granted-feats SkillInfoBox popup is still on the modal stack.
- L215 `void NoteLevelUpOpened(void* gui)`
- L237 `bool PlayerCanLevelUp()`
- L264 `bool CreatureCanLevelUp(void* serverCreature)`
- L279 `bool TriggerLevelUp()`
- L361 `bool IsOpeningLevelUp()`
- L363 `void TickLevelUpPause()`
