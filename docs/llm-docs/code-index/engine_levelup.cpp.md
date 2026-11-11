# engine_levelup.cpp (389 lines)

Source: `patches/Accessibility/engine_levelup.cpp`

the action menu, WITHOUT opening a sub-screen

## Declarations (in source order)

- L106 `void* GetInGameCharacterPanel(void* gui)`: Read the live CSWGuiInGameCharacter pointer from the CGuiInGame singleton.
- L114 `bool SetLevelUpMode(void* gui, int mode)`: Set CGuiInGame.level_up_mode via the engine's own setter.
- L150 `void DriveSWGuiStatus(void* gui, int status, int p2)`
- L197 `bool GrantedFeatsPopupStillModal()`: True while the granted-feats SkillInfoBox popup is still on the modal stack.
- L209 `void NoteLevelUpOpened(void* gui)`
- L231 `bool PlayerCanLevelUp()`
- L258 `bool CreatureCanLevelUp(void* serverCreature)`
- L273 `bool TriggerLevelUp()`
- L355 `bool IsOpeningLevelUp()`
- L357 `void TickLevelUpPause()`
