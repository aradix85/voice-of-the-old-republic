# mod_settings_store.cpp (154 lines)

Source: `patches/Accessibility/mod_settings_store.cpp`

(From mod_settings_store.h.) Persistent mod-settings store.

A tiny `key=value` text file at <install>\acc_settings.ini (next to the logs\ dir), loaded lazily on first access and rewritten in full on every Set (the file is a handful of lines, so a full rewrite is trivial and keeps the on-disk state always consistent).

This exists because the Mod-Einstellungen toggles and the cue-volume slider are otherwise in-memory only — they reset to their defaults every launch. Owners (menus_modsettings for the toggles, audio_bus for the cue volume) pull their persisted value on first use and push back here on change; this module is just typed get/set over the file.

Defaults: a missing key (or a missing/unreadable file) returns the caller- supplied default, so first-run behaviour is unchanged. Thread-safe (one mutex); accesses are main-thread in practice but cheap to guard.

## Declarations (in source order)

- L26 `bool ResolvePath()`: \acc_settings.ini, derived from acclog::PatchDir() (= \ patches) by stripping the last path component.
- L39 `void EnsureLoaded()`: Must hold g_mtx.
- L72 `void Save()`: Must hold g_mtx.
- L88 `bool GetBool(const char* key, bool defValue)`
- L100 `int GetInt(const char* key, int defValue)`
- L112 `void SetBool(const char* key, bool value)`
- L121 `void SetInt(const char* key, int value)`
- L132 `bool GetStr(const char* key, char* outBuf, int bufSize)`
- L144 `void SetStr(const char* key, const char* value)`
