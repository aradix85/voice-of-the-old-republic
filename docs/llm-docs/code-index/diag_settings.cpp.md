# diag_settings.cpp (275 lines)

Source: `patches/Accessibility/diag_settings.cpp`

Startup snapshot — see diag_settings.h. Runs once from OnRulesInit, after loader lock is long past so file I/O + heap alloc are safe.

## Declarations (in source order)

- L25 `bool ResolveInstallRelative(const char* rel, char* out, size_t outCap)`
- L36 `void DumpIni(const char* iniPath)`
- L75 `void ProbeFile(const char* rel)`
- L101 `int CountFiles(const char* relDir)`
- L139 `void LogMilesProvider(void* soundInternal)`: Reads the engine's provider handle and walks Miles' provider list.
- L200 `void LogDirectSoundModule()`: Where dsound.dll was loaded from: the game folder means dsoal, System32 means Windows' own DirectSound.
- L214 `void LogAlsoftConfig()`: The alsoft.ini keys that decide HRTF.
- L231 `void LogAudioPathOnce()`
- L242 `void LogStartupSnapshot()`
