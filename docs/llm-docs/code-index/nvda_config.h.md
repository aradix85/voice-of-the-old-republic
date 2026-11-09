# nvda_config.h (44 lines)

Source: `patches/Accessibility/nvda_config.h`

Read-only view of NVDA's saved configuration (%APPDATA%\nvda\nvda.ini).

Lets the mod imitate the screen reader's voice for positional speech: which synthesizer NVDA uses and with which settings. NVDA offers no API to query its live settings (its controller client only accepts text), so this is the SAVED state — it changes when NVDA saves its config (NVDA+Ctrl+C, or on exit with "save configuration on exit"). Re-loaded on every call; the file is a few KB.

nvda.ini is ConfigObj: "[section]", indented "[[subsection]]", "key = value".

## Declarations (in source order)

- L19 `struct Entry`
- L26 `struct Config`
- L38 `std::string ConfigDir()`: %APPDATA%\nvda — the installed NVDA's config folder (a portable NVDA keeps its own elsewhere and is not found).
- L41 `bool Load(Config& out)`: Parses nvda.ini.
