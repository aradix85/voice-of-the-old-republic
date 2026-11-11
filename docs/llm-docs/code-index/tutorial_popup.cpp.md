# tutorial_popup.cpp (223 lines)

Source: `patches/Accessibility/tutorial_popup.cpp`

(From tutorial_popup.h.) On-demand tutorial popup for the voice-acted Endar-Spire tutorial lines (Trask / end_pop*). Those lines stay suppressed during their Basic VO; when the dialogue reaches a reply prompt (the game's own break), we mount a REAL engine tutorial popup carrying the keyboard hint, so it pauses and dismisses exactly like a stock game tutorial.

Mechanism (see scratchpad trask_popup_recipe.md / docs): * Clear the tutorial once-shown bit for a fixed reason, then call CGuiInGame::ShowTutorialWindow directly (bypasses the funnel's "no tutorials during dialogue" gate) to mount the box. * Set the box's visible text to the line's original strref (mouse wording) via CSWGuiMessageBox::SetMessage(strref) — mirrors the Surface-1 split. * Route the SPOKEN text to our keyboard hint via the synthetic flag below (the existing TutorialBox reader in menus_monitors honours it).

## Declarations (in source order)

- L90 `void SetPause(bool on)`
- L100 `bool TutorialBoxPresent()`: Is a TutorialBox panel currently in the GUI manager's stack?
- L105 `void FirePopup(uint32_t strref, const char* hint)`
- L157 `static bool PendingContainsHint(const char* hint)`: True if `hint` is already one of the newline-separated segments accumulated this break.
- L170 `void RecordPendingHint(uint32_t strref, const char* hint)`
- L190 `bool FirePendingAtReplyBreak()`
- L206 `bool SyntheticActive()`
- L207 `const char* SyntheticHint()`
- L209 `void PollDismiss()`
