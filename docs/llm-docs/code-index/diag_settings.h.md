# diag_settings.h (27 lines)

Source: `patches/Accessibility/diag_settings.h`

Startup snapshot of swkotor.ini and install-root layout. Logged once at session start so support bundles always carry a baseline of the user's config — no need to chase a follow-up message asking what's in their ini.

## Declarations (in source order)

- L12 `void LogStartupSnapshot()`: Walk every [section] / key=value pair in \swkotor.ini and emit one log line per entry.
- L24 `void LogAudioPathOnce()`: Once the game's sound system is up, logs the audio path: where dsound.dll came from, which Miles 3D provider the game opened (only DirectSound3D / EAX hand 3D sounds to dsoal; Miles' software providers mix them to ste...
