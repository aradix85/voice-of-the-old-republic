# audio_footstep_suppress.h (73 lines)

Source: `patches/Accessibility/audio_footstep_suppress.h`

Stuck-detection via footstep suppression.

When the engine plays a walk animation (which fires PlayFootstep) but the character isn't actually moving (sub-epsilon displacement between ticks), we suppress the footstep. Silence signals "stuck" to a blind player without introducing a new cue.

Mechanism: - Tick() samples GetPlayerPosition each frame; sets g_was_stuck if displacement < kStuckEpsilonMeters. - OnPlayFootstep detour returns 1 (consume via LAB_0061a632 early- return — skips audio + footprint + rumble together) when the creature is the player leader AND g_was_stuck.

NPC footsteps must NOT be suppressed; the handler gates on GetClientLeader identity, same check the engine uses internally for priority-group assignment.

CRITICAL: on both games the hook site is mid-function at the engine's own early-out branch (K1 JZ, K2 CMP+JNZ — with INVERTED return polarity; see the handler comment in the .cpp). Earlier K1 attempts at "cleaner" entry-point hooks silenced all footsteps because cut bytes landed across TEST EAX,EAX, clobbering ZF before the engine's downstream JZ. See hooks.toml / kotor2.hooks.toml around OnPlayFootstep for the forensic detail and the EFLAGS / EAX wrapper-bug notes. Do not "simplify" without re-reading.

was_stuck reflects the PREVIOUS tick's displacement (~33ms lag at 30Hz).

Stuck-direction probe: when the leader is animating walk AND net displacement over the last 2 seconds is < 0.5 m, runs an 8-cardinal probe and speaks clear directions once per stuck episode. Rearms when the player makes >0.5 m progress.

## Declarations (in source order)

- L42 `void Tick()`: Self-gates on player resolved; idempotent.
- L46 `bool WasStuckLastTick()`: Was the most-recent Tick a sub-epsilon displacement?
- L55 `bool StuckSustainedFor(unsigned int min_ms)`: True iff the stuck state has been continuously held for at least min_ms.
- L59 `void NoteLeaderFootstep()`: Stamps the timestamp the stuck-direction probe gates on.
- L70 `uint64_t LastLeaderFootstepMs()`: GetTickCount64() at the last leader footstep, 0 if none this session.
