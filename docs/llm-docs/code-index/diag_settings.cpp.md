# diag_settings.cpp (271 lines)

Source: `patches/Accessibility/diag_settings.cpp`

Startup snapshot — see diag_settings.h. Runs once from OnRulesInit, after loader lock is long past so file I/O + heap alloc are safe.

## Declarations (in source order)

- L26 `bool ResolveInstallRelative(const char* rel, char* out, size_t outCap)`
- L37 `void DumpIni(const char* iniPath)`
- L76 `void ProbeFile(const char* rel)`
- L102 `int CountFiles(const char* relDir)`
- L140 `void LogMilesProvider(void* soundInternal)`: Reads the engine's provider handle and walks Miles' provider list.
- L196 `void LogDirectSoundModule()`: Where dsound.dll was loaded from: the game folder means dsoal, System32 means Windows' own DirectSound.
- L210 `void LogAlsoftConfig()`: The alsoft.ini keys that decide HRTF.
- L227 `void LogAudioPathOnce()`
- L238 `void LogStartupSnapshot()`
