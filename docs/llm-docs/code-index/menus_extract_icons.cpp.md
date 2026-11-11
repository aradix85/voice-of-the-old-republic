# menus_extract_icons.cpp (159 lines)

Source: `patches/Accessibility/menus_extract_icons.cpp`

Image-only controls of the in-game screens, named from what they stand for: the party portraits, the in-game menu icons and the map's note arrows.

## Declarations (in source order)

- L55 `const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable)`: CSWGuiPartySelectionButton: OnPanelAdded (0x006beeb0) gives each portrait the companion's image and no caption, so it is read through ReadPartyPortrait (engine_player.h).
- L112 `const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize)`: No engine path carries the in-game menu icons' text (inline, strref, text_object, gui_string and tooltip are all empty), so they are named from the table by .gui id.
- L140 `const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize)`: CSWGuiInGameMap's two image-only buttons beside the map (up_button, down_button): HandleInputEvent 0x31 / 0x32, which the map routes to CSWGuiMapHider::GetPrevMapNote / GetNextMapNote, cycling the explored map notes.
