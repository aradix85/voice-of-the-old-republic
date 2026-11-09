# diag_settings.cpp (282 lines)

Source: `patches/Accessibility/diag_settings.cpp`

Startup snapshot — see diag_settings.h. Runs once from OnRulesInit, after loader lock is long past so file I/O + heap alloc are safe.

## Declarations (in source order)

- L24 `bool ResolveInstallRelative(const char* rel, char* out, size_t outCap)`
- L35 `void DumpIni(const char* iniPath)`
- L74 `void ProbeFile(const char* rel)`
- L100 `int CountFiles(const char* relDir)`
- L136 `bool ContainsNoCase(const char* hay, const char* needle)`
- L147 `void LogMilesProvider(void* soundInternal)`: Reads the engine's provider handle and walks Miles' provider list.
- L207 `void LogDirectSoundModule()`: Where dsound.dll was loaded from: the game folder means dsoal, System32 means Windows' own DirectSound.
- L221 `void LogAlsoftConfig()`: The alsoft.ini keys that decide HRTF.
- L238 `void LogAudioPathOnce()`
- L249 `void LogStartupSnapshot()`
