# tutorial_popup.h (43 lines)

Source: `patches/Accessibility/tutorial_popup.h`

On-demand tutorial popup for the voice-acted Endar-Spire tutorial lines (Trask / end_pop*). Those lines stay suppressed during their Basic VO; when the dialogue reaches a reply prompt (the game's own break), we mount a REAL engine tutorial popup carrying the keyboard hint, so it pauses and dismisses exactly like a stock game tutorial.

Mechanism (see scratchpad trask_popup_recipe.md / docs): * Clear the tutorial once-shown bit for a fixed reason, then call CGuiInGame::ShowTutorialWindow directly (bypasses the funnel's "no tutorials during dialogue" gate) to mount the box. * Set the box's visible text to the line's original strref (mouse wording) via CSWGuiMessageBox::SetMessage(strref) — mirrors the Surface-1 split. * Route the SPOKEN text to our keyboard hint via the synthetic flag below (the existing TutorialBox reader in menus_monitors honours it).

## Declarations (in source order)

- L25 `void RecordPendingHint(uint32_t strref, const char* hint)`: Remember the keyboard hint (and its source strref, for the popup's visible text) for the most recent rewritten tutorial dialogue line.
- L30 `bool FirePendingAtReplyBreak()`: Called when a dialogue reply becomes navigable.
- L35 `bool SyntheticActive()`: True while our synthetic tutorial popup is on screen — the TutorialBox speech paths (fingerprint override, listbox suppression, chain gate) route the hint and keep the mouse-worded message off the arrow-nav chain.
- L36 `const char* SyntheticHint()`
- L40 `void PollDismiss()`: Per-tick: detect the synthetic popup closing (OK) and clear state + unpause.
