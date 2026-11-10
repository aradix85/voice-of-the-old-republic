# audio_footstep_suppress.cpp (336 lines)

Source: `patches/Accessibility/audio_footstep_suppress.cpp`

(keyboard walk keys OR the KOTOR 2 left stick)

## Declarations (in source order)

- L72 `bool IsWalking(uint64_t now_ms)`: Walk-anim freshness: a leader footstep within kFootstepFreshnessMs.
- L79 `bool WasStuckLastTick()`
- L81 `bool StuckSustainedFor(unsigned int min_ms)`
- L86 `void NoteLeaderFootstep()`
- L90 `uint64_t LastLeaderFootstepMs()`
- L92 `void Tick()`
- L211 `extern "C" int __cdecl OnPlayFootstep(void* creature)`: CSWCCreature::PlayFootstep detour — BOTH games.
- L317 `extern "C" int __cdecl OnUpdateRollingFootstep(void* creature)`
