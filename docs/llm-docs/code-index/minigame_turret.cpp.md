# minigame_turret.cpp (2449 lines)

Source: `patches/Accessibility/minigame_turret.cpp`

Turret (space-combat gunner) minigame accessibility — see turret_game.h for design.

Shares CSWMiniGame with swoop racing; distinguished by type==2 (swoop==1), read at CSWMiniGame +0x80. The aim is CSWMiniPlayer.offset (+0x1c4): offset.x = elevation°, offset.z = azimuth° — engine-confirmed from the decompile of CSWMiniPlayer::Control (it integrates offset and pushes it into the rotating gun/camera models each tick). The crosshair is the gun's bullethook0 node world direction (ReadAimLine); aim-assist WRITES offset (WriteOffset). See docs/llm-docs/turret-minigame-model.md.

What it does: - Entry/exit announce (the native control hint — WASD aims, Space fires; both are native keyboard actions, live-confirmed). - ONE 3D crosshair cue per round, on the SELECTED fighter (Q/E to lock / cycle), using a RISING TICK RATE for centring: * OFF-TARGET — the cue PULSES, panned toward the (lead-corrected) target so the player can swing onto it from anywhere. The pulse interval shrinks as the aim closes on the hit cone (slow pings far off, near-buzz at the edge); behind, the pulses pin to the shorter- swing side. * ON-TARGET — the cue goes SOLID (continuous tone) the instant the aim is inside the range-scaled hitbox. Solid == a shot connects, at any range. (Loudness still swells toward dead-centre within the cone as a secondary cue; it is NOT the primary signal — the engine's distance→ gain curve has too steep a knee to track the cone by loudness alone.) Aim point is the INTERCEPT (range-gated lead; see Arena) so distance shots lead the target. Earlier we ran a SECOND ambient engine-loop on the nearest fighters ("hum"); play-testing showed it was mostly ignored, so it was dropped in favour of this single, less-cluttered cue. The vanilla fighters carry no engine loop of their own (CSWTrackFollower+0x144 null); the engine still plays its own one-shot mgs_sith_hit / _expl on hit/kill, which covers "you connected" without us adding anything.

Both games. KOTOR 1 has one turret area (M12ab); KOTOR 2 has three, and they differ in bolt speed, fighter size, fighter paths and aim limits (see docs/llm-docs/k2-turret-model.md). So the bolt speed / lifespan and the aim limits are READ from the engine at entry (the "arena"), and every range the cues use is derived from them through a model calibrated on KOTOR 1 — see "Range model" below. KOTOR 2's Peragus hangar turret (107PER) has no minigame fighters at all: its targets are ordinary area creatures and barrels, and its bolts do not travel, so it runs the same cues on an area census with the lead switched off ("area-target mode").

## Declarations (in source order)

- L725 `void* ReadMiniGameViaArea()`: Read CSWCArea.mini_game via the player-area chain.
- L733 `bool IsTurretMiniGame(void* mg)`: True iff `mg` is a live CSWMiniGame reporting the turret type.
- L737 `bool LatchedStillValid()`
- L760 `void StopAllLoops()`
- L829 `Vector QuatRotate(const Quat& q, const Vector& v)`: Rotate a vector by a quaternion (w,x,y,z): v' = v + 2w(u×v) + 2(u×(u×v)), u = (x,y,z).
- L843 `void* ResolveGunBank0(void* miniGame)`: Resolve the player's first gun bank (CSWMGGunBank*).
- L853 `void* ResolveGunModel()`: Resolve the player's first gun model (CAurObject*) for the bullethook read.
- L865 `void ReadArena(void* miniGame)`: Read the area's ballistics and aim limits into g_state.arena.
- L917 `float MakeHorizonSec(float radius, float speed)`: How far ahead (seconds of bolt flight) the lead can predict a target of this radius and speed before its miss outgrows the hitbox.
- L929 `float CensusRangeM()`: Census / Q/E range.
- L938 `float KillableRangeM(float radius, float speed)`: Distance inside which the range cue says "you can hit this".
- L946 `float HumCompression()`
- L953 `float TargetSpeed(const TargetInfo* info, float measured)`: The speed the range model should use for a target: the engine's track speed for a fighter, else the measured one.
- L965 `bool ReadAimLine(Vector& outDir, Vector& outOrigin)`: THE CROSSHAIR.
- L993 `bool ReadOffset(float& azOut, float& elOut)`: Read / write the aim field (CSWMiniPlayer.offset +0x1c4): x = elevation°, z = azimuth°.
- L1002 `void WriteOffset(float az, float el)`
- L1013 `float AngleBetweenDeg(const Vector& a, const Vector& b)`: Angle (degrees) between the aim ray and the (already normalised) direction to a target.
- L1024 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: ----- LeadCheck self-measuring diagnostic ----- Enqueue this tick's linear & curve intercepts (world frame) tagged with the bolt-arrival time; when one matures, compare both to the fighter's ACTUAL position then and l...
- L1030 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L1047 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: Find a slot's index in the (nearest-first) census, or -1.
- L1054 `int NumberForSlot(int slot)`: Stable "Fighter N" number for an MGO slot, assigned on first request and fixed for the round (see State::slot_number).
- L1066 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`: Play the range-boundary marker (c_drdastro_atk1) at a compressed, always- audible point toward the fighter so it pans correctly — the same trick the lock locator uses.
- L1087 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak = true)`: Lock onto census entry `idx`: set the selection, speak "Fighter N, D metres", and fire a LOUD one-shot positional cue toward the fighter so the player hears which way to swing — independent of where the aim currently ...
- L1154 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`: Q/E target cycle.
- L1187 `void DriveSelectedPeg(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& aimDir, bool haveAim, const Vector& listener, bool selectedAliveInPool)`: Drive the continuous peg tone on the selected fighter.
- L1905 `void Reset()`: In place: the entry arrays are only read up to `count`.
- L1912 `void AddToCensus(Census& c, int slot, const Vector& pos, float distSq, const TargetInfo& info)`
- L1924 `void TrackSlotHp(int slot, int hp, int maxHp)`: Per-slot HP bookkeeping shared by both census sources: a drop is a hit on that target (logged even when it isn't the locked one), and "ever damaged" lets the down-detector tell a kill from a despawn.
- L1936 `void CensusFighters(Census& c, const Vector& listener)`: Minigame fighters: every MGO slot that resolves AsEnemy.
- L1973 `int AreaSlotFor(void* obj, uint32_t handle, bool isCreature)`: The pseudo-slot of an area target, assigned on first sight with the engine's name as its noun and a number counted per noun ("Explosive barrel 3", "Sith trooper 7").
- L1999 `void CensusAreaTargets(Census& c, const Vector& listener)`: Area-target mode (107PER): hostile creatures and non-plot placeables in the player's area.
- L2037 `void AnnounceTargetGone(int slot, bool diedOnScreen)`: Speak a target that left the field: destroyed, or — an area creature that vanished while alive (107PER's troopers are removed when they reach the ship) — got away.
- L2059 `void TickFighterCues()`
- L2189 `void AnnounceEntry()`
- L2201 `void AnnounceExit()`
- L2211 `void HandleEnter(void* mg)`
- L2273 `void HandleExit()`
- L2328 `extern "C" void __cdecl OnTurretBulletHit(void* hitEvent)`: TurretHitGeom diagnostic handler — hooked at CSWMiniGameObject::OnHitBullet (@0x0066c190), which fires once per engine-CONFIRMED bullet hit.
- L2369 `extern "C" void __cdecl OnPlayerFire(void* /*player*/)`: Player fire counter (hooked at CSWMiniPlayer::Fire @0x0066dc50).
- L2375 `bool IsActive()`
- L2377 `void Tick()`
