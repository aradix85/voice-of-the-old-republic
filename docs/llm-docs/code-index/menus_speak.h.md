# menus_speak.h (18 lines)

Source: `patches/Accessibility/menus_speak.h`

Shared speech+log helper for the per-menu Speak* paths (currently only unified_action_menu — same shape: speak a label with interrupt, then log it under the menu's tag with a context printf string).

## Declarations (in source order)

- L14 `void SpeakChoice(const char* tag, const char* label, const char* ctxFmt, ...)`: Speak `label` (interrupt=true) and log it under `tag` with a printf- formatted context.
