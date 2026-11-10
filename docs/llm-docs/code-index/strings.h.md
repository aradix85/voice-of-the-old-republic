# strings.h (2422 lines)

Source: `patches/Accessibility/strings.h`

The mod's spoken text: one Id per line the user hears; Get() resolves it to the English string in strings.cpp.

Logs stay out of this table; only spoken strings use Get(). Format-string Ids use printf placeholders.

## Declarations (in source order)

- L11 `enum class Id`
- L2419 `const char* Get(Id id)`: Resolve a string Id.
