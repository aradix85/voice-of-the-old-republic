# audio_footstep_suppress.cpp (183 lines)

Source: `patches/Accessibility/audio_footstep_suppress.cpp`

Footstep suppression while the leader is stuck — see audio_footstep_suppress.h.

## Declarations (in source order)

- L30 `bool IsWalking(uint64_t now_ms)`
- L36 `bool StuckSustainedFor(unsigned int min_ms)`: Stuck held without a break for at least min_ms.
- L43 `uint64_t LastLeaderFootstepMs()`
- L45 `void Tick()`
- L102 `extern "C" int __cdecl OnPlayFootstep(void* creature)`: CSWCCreature::PlayFootstep detour, both games.
- L167 `extern "C" int __cdecl OnUpdateRollingFootstep(void* creature)`
