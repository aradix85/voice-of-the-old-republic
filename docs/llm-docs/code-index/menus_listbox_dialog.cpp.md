# menus_listbox_dialog.cpp (112 lines)

Source: `patches/Accessibility/menus_listbox_dialog.cpp`

Dialogue reply lists: the conversation, cinematic and computer panels.

## Declarations (in source order)

- L16 `bool DialogCinematicMatches(void* p)`
- L20 `bool DialogCinematicCopyMatches(void* p)`
- L24 `bool DialogComputerMatches(void* p)`
- L28 `bool DialogComputerCameraMatches(void* p)`
- L32 `void* DialogFindRepliesLb(void* p)`
- L69 `bool DialogRepliesLive(void* /*panel*/)`: The engine populates the CGuiInGame reply store (SetReplyData) only at the reply break — when the entry's VO ends and the choices actually appear.
- L77 `constexpr ListBoxPanelSpec MakeDialogSpec(const char* logTag, bool (*matches)(void* panel))`: All four dialog variants take the identical spec — same reply-listbox locator, no announce (see above), no Enter/Esc/title handling.
