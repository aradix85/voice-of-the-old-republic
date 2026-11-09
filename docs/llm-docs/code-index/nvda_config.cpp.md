# nvda_config.cpp (74 lines)

Source: `patches/Accessibility/nvda_config.cpp`

(From nvda_config.h.) Read-only view of NVDA's saved configuration (%APPDATA%\nvda\nvda.ini).

Lets the mod imitate the screen reader's voice for positional speech: which synthesizer NVDA uses and with which settings. NVDA offers no API to query its live settings (its controller client only accepts text), so this is the SAVED state — it changes when NVDA saves its config (NVDA+Ctrl+C, or on exit with "save configuration on exit"). Re-loaded on every call; the file is a few KB.

nvda.ini is ConfigObj: "[section]", indented "[[subsection]]", "key = value".

## Declarations (in source order)

- L12 `std::string Trim(const std::string& s)`
- L20 `std::string Unbracket(const std::string& t)`: "[[name]]" / "[name]" → "name".
- L29 `const char* Config::Find(const char* section, const char* subsection, const char* key) const`
- L38 `int Config::FindInt(const char* section, const char* subsection, const char* key, int fallback) const`
- L44 `std::string ConfigDir()`
- L51 `bool Load(Config& out)`
