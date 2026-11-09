# minigame_turret_targets.cpp (156 lines)

Source: `patches/Accessibility/minigame_turret_targets.cpp`

The selected target: its announcement with range cue, cycling targets, and the lead probes that measure where to aim ahead of it.

## Declarations (in source order)

- L13 `void PushLeadProbe(int slot, const Vector& linW, const Vector& curveW, ULONGLONG dueMs)`: ----- LeadCheck self-measuring diagnostic ----- Enqueue this tick's linear & curve intercepts (world frame) tagged with the bolt-arrival time; when one matures, compare both to the fighter's ACTUAL position then and l...
- L19 `void DrainLeadProbes(int slot, const Vector& fighterWorldNow, ULONGLONG now)`
- L39 `Vector PlayRangeCueAt(const Vector& targetPos, float dist, const Vector& listener)`: Play the range-boundary marker (c_drdastro_atk1) at a compressed, always- audible point toward the fighter so it pans correctly — the same trick the lock locator uses.
- L60 `void AnnounceSelectedTarget(int idx, const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener, bool speak)`: Lock onto census entry `idx`: set the selection, speak "Fighter N, D metres", and fire a LOUD one-shot positional cue toward the fighter so the player hears which way to swing — independent of where the aim currently ...
- L127 `void HandleTargetCycle(const int occSlot[], const float occDist[], const Vector occPos[], const TargetInfo occInfo[], int occCount, const Vector& listener)`: Q/E target cycle.
