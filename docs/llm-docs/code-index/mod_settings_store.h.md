# mod_settings_store.h (39 lines)

Source: `patches/Accessibility/mod_settings_store.h`

Persistent mod-settings store.

A tiny `key=value` text file at <install>\acc_settings.ini (next to the logs\ dir), loaded lazily on first access and rewritten in full on every Set (the file is a handful of lines, so a full rewrite is trivial and keeps the on-disk state always consistent).

This exists because the Mod settings toggles and the cue-volume slider are otherwise in-memory only — they reset to their defaults every launch. Owners (menus_modsettings for the toggles, audio_bus for the cue volume) pull their persisted value on first use and push back here on change; this module is just typed get/set over the file.

Defaults: a missing key (or a missing/unreadable file) returns the caller- supplied default, so first-run behaviour is unchanged. Thread-safe (one mutex); accesses are main-thread in practice but cheap to guard.

## Declarations (in source order)

- L22 `bool GetBool(const char* key, bool defValue)`
- L23 `int GetInt (const char* key, int defValue)`
- L25 `void SetBool(const char* key, bool value)`
- L26 `void SetInt (const char* key, int value)`
- L35 `bool GetStr(const char* key, char* outBuf, int bufSize)`: String-typed access.
- L36 `void SetStr(const char* key, const char* value)`
