# audio_footstep_suppress.cpp (623 lines)

Source: `patches/Accessibility/audio_footstep_suppress.cpp`

(keyboard walk keys OR the KOTOR 2 left stick)

## Declarations (in source order)

- L140 `void RunStuckProbe(const Vector& pos)`
- L262 `void TickCirclingAnnounce(const Vector& pos, uint64_t now_ms, bool walking)`: Fires when walking but oscillating: cumulative path length to net displacement ratio is high over a 4s window.
- L320 `void TickStuckAnnounce(const Vector& pos, uint64_t now_ms)`
- L358 `bool WasStuckLastTick()`
- L360 `bool StuckSustainedFor(unsigned int min_ms)`
- L365 `void NoteLeaderFootstep()`
- L369 `uint64_t LastLeaderFootstepMs()`
- L371 `void Tick()`
- L498 `extern "C" int __cdecl OnPlayFootstep(void* creature)`: CSWCCreature::PlayFootstep detour — BOTH games.
- L604 `extern "C" int __cdecl OnUpdateRollingFootstep(void* creature)`
