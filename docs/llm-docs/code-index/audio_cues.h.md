# audio_cues.h (96 lines)

Source: `patches/Accessibility/audio_cues.h`

Navigation-cue vocabulary — resref mapping for the audio slots.

Data-only header. Consumers map enum → resref via GetNavCueResref then call audio::PlayCue / PlayCue3D.

Swapping a resref: change the string literal on the case below. CResRef has a 16-char hard limit (silent truncation past that → resolution miss → silent cue). Resolution is case-insensitive through Override → streamwaves → streamsounds → streammusic → BIF/RIM, so an Override/ WAV with the same resref transparently shadows the stock asset.

Verify a resref resolves before using it: Play3DOneShotSound silently no-ops on a miss. build/sounds-extracted-full/ is the truth table.

## Declarations (in source order)

- L19 `enum class NavCue`
- L60 `constexpr const char* GetNavCueResref(NavCue cue)`
