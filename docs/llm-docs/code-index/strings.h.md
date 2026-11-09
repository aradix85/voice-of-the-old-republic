# strings.h (2505 lines)

Source: `patches/Accessibility/strings.h`

User-facing string table.

Layer: i18n/ — sits between the speech path (cycle_input, future announce paths) and Prism. Centralises every string the user hears so language swaps + wording changes happen in one place.

Logs stay English regardless of the active language: developer-readable log lines (acc::filter::CategoryName etc.) are NOT routed through this table. Only spoken strings use Get().

Encoding: source files use 7-bit ASCII for English and Windows-1252 hex escapes (e.g. "T\xFCr") for German, so the literal bytes already match Prism's ANSI overload (which calls MultiByteToWideChar(CP_ACP) — CP_ACP = Windows-1252 on a German Windows install). UTF-8 source would also work but only if the build script sets /utf-8 explicitly, which create-patch.bat does not.

Format-string IDs use printf-style placeholders. The argument order is stable across languages (`%s` first for the name, `%d` for clock, `%d` for metres) — German just keeps the same order with different unit words and the "auf X Uhr" navigation idiom.

## Declarations (in source order)

- L27 `enum class Id`
- L2456 `enum class Lang`
- L2477 `unsigned CodepageFor(Lang l)`: Windows codepage the language's narrow string bytes are written in.
- L2482 `void SetLanguage(Lang l)`: Set the active language.
- L2483 `Lang GetLanguage()`
- L2488 `const char* Get(Id id)`: Resolve a string ID to its language-specific bytes.
