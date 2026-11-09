# menus_speak.cpp (27 lines)

Source: `patches/Accessibility/menus_speak.cpp`

(From menus_speak.h.) Shared speech+log helper for the per-menu Speak* paths (currently only unified_action_menu — same shape: speak a label with interrupt, then log it under the menu's tag with a context printf string).

## Declarations (in source order)

- L11 `void SpeakChoice(const char* tag, const char* label, const char* ctxFmt, ...)`
