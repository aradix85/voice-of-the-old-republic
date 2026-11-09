# minigame_turret.cpp (420 lines)

Source: `patches/Accessibility/minigame_turret.cpp`

Turret (space-combat gunner) minigame accessibility — see turret_game.h for design.

Shares CSWMiniGame with swoop racing; distinguished by type==2 (swoop==1), read at CSWMiniGame +0x80. The aim is CSWMiniPlayer.offset (+0x1c4): offset.x = elevation°, offset.z = azimuth° — engine-confirmed from the decompile of CSWMiniPlayer::Control (it integrates offset and pushes it into the rotating gun/camera models each tick). The crosshair is the gun's bullethook0 node world direction (ReadAimLine); aim-assist WRITES offset (WriteOffset). See docs/llm-docs/turret-minigame-model.md.

What it does: - Entry/exit announce (the native control hint — WASD aims, Space fires; both are native keyboard actions, live-confirmed). - ONE 3D crosshair cue per round, on the SELECTED fighter (Q/E to lock / cycle), using a RISING TICK RATE for centring: * OFF-TARGET — the cue PULSES, panned toward the (lead-corrected) target so the player can swing onto it from anywhere. The pulse interval shrinks as the aim closes on the hit cone (slow pings far off, near-buzz at the edge); behind, the pulses pin to the shorter- swing side. * ON-TARGET — the cue goes SOLID (continuous tone) the instant the aim is inside the range-scaled hitbox. Solid == a shot connects, at any range. (Loudness still swells toward dead-centre within the cone as a secondary cue; it is NOT the primary signal — the engine's distance→ gain curve has too steep a knee to track the cone by loudness alone.) Aim point is the INTERCEPT (range-gated lead; see Arena) so distance shots lead the target. Earlier we ran a SECOND ambient engine-loop on the nearest fighters ("hum"); play-testing showed it was mostly ignored, so it was dropped in favour of this single, less-cluttered cue. The vanilla fighters carry no engine loop of their own (CSWTrackFollower+0x144 null); the engine still plays its own one-shot mgs_sith_hit / _expl on hit/kill, which covers "you connected" without us adding anything.

Both games. KOTOR 1 has one turret area (M12ab); KOTOR 2 has three, and they differ in bolt speed, fighter size, fighter paths and aim limits (see docs/llm-docs/k2-turret-model.md). So the bolt speed / lifespan and the aim limits are READ from the engine at entry (the "arena"), and every range the cues use is derived from them through a model calibrated on KOTOR 1 — see "Range model" below. KOTOR 2's Peragus hangar turret (107PER) has no minigame fighters at all: its targets are ordinary area creatures and barrels, and its bolts do not travel, so it runs the same cues on an area census with the lead switched off ("area-target mode").

## Declarations (in source order)

- L118 `void* ReadMiniGameViaArea()`: Read CSWCArea.mini_game via the player-area chain.
- L126 `bool IsTurretMiniGame(void* mg)`: True iff `mg` is a live CSWMiniGame reporting the turret type.
- L130 `bool LatchedStillValid()`
- L149 `void StopAllLoops()`
- L160 `void AnnounceEntry()`
- L172 `void AnnounceExit()`
- L182 `void HandleEnter(void* mg)`
- L244 `void HandleExit()`
- L299 `extern "C" void __cdecl OnTurretBulletHit(void* hitEvent)`: TurretHitGeom diagnostic handler — hooked at CSWMiniGameObject::OnHitBullet (@0x0066c190), which fires once per engine-CONFIRMED bullet hit.
- L340 `extern "C" void __cdecl OnPlayerFire(void* /*player*/)`: Player fire counter (hooked at CSWMiniPlayer::Fire @0x0066dc50).
- L346 `bool IsActive()`
- L348 `void Tick()`
