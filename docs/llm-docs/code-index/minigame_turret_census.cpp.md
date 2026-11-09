# minigame_turret_census.cpp (338 lines)

Source: `patches/Accessibility/minigame_turret_census.cpp`

Which targets are there: the census of fighters and area targets with their numbers and hit points, a target that is gone, and the per-tick fighter cues that drive the selected target.

## Declarations (in source order)

- L43 `void Reset()`: In place: the entry arrays are only read up to `count`.
- L50 `void AddToCensus(Census& c, int slot, const Vector& pos, float distSq, const TargetInfo& info)`
- L62 `void TrackSlotHp(int slot, int hp, int maxHp)`: Per-slot HP bookkeeping shared by both census sources: a drop is a hit on that target (logged even when it isn't the locked one), and "ever damaged" lets the down-detector tell a kill from a despawn.
- L74 `void CensusFighters(Census& c, const Vector& listener)`: Minigame fighters: every MGO slot that resolves AsEnemy.
- L111 `int AreaSlotFor(void* obj, uint32_t handle, bool isCreature)`: The pseudo-slot of an area target, assigned on first sight with the engine's name as its noun and a number counted per noun ("Explosive barrel 3", "Sith trooper 7").
- L137 `void CensusAreaTargets(Census& c, const Vector& listener)`: Area-target mode (107PER): hostile creatures and non-plot placeables in the player's area.
- L175 `void AnnounceTargetGone(int slot, bool diedOnScreen)`: Speak a target that left the field: destroyed, or — an area creature that vanished while alive (107PER's troopers are removed when they reach the ship) — got away.
- L200 `int CensusIndexOf(const int occSlot[], int occCount, int slot)`: Find a slot's index in the (nearest-first) census, or -1.
- L207 `int NumberForSlot(int slot)`: Stable "Fighter N" number for an MGO slot, assigned on first request and fixed for the round (see State::slot_number).
- L215 `void TickFighterCues()`
