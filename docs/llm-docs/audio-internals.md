# Audio & sound-source internals (RE reference)

Play3DOneShotSound gain chain, CExoSoundSource lifecycle, sound-mode pause exemptions, footstep paths, cue/party filtering, and droid-subtitle handling.

> Migrated from the agent memory store on 2026-06-14 (memory-system cleanup).
> Each section below is one former memory note, preserved verbatim. Verify
> addresses/offsets against current code before relying.

## play3doneshotsound_volume_amplifies
_How a one-shot cue's final loudness is computed; corrects the discredited \"4.0x amplifies\" claim. SetVolume clamps per-source byte to 127; group volume is the unclamped lever._


CORRECTS the earlier claim that Play3DOneShotSound's float arg amplifies >1.0.
That was wrong: the float was being fed into `fixed_variance` (a pitch field)
under a mislabelled typedef and then zeroed by the pitch-neutraliser detour, so
`kAccCueGain=4.0` did NOTHING to volume. Today `PlayCue`/`PlayCue3D` take a
`baseVolume` byte that `EffectiveVolumeByte` (audio_bus.cpp) scales by the
cue-volume slider and writes as `volume_byte` (lever 1 below).

**Verified gain chain (Ghidra, 2026-06-01).** Final hardware amplitude is computed
in `CExoSoundSourceInternal::SetVolume` @0x005db800, re-invoked at playback from
`PlaySourceOn2DVoice`/`PlaySourceOn3DVoice`:
- `_Var7 = priority_group_volume × src_vol × factor / 127`  (factor=1.0 unless group 4 or 0x15-2D)
- 2D: `amp = _Var7 × sliderVol / 127`
- 3D: `amp = pow(_Var7 × sliderVol / 127, 0.6)`  ← loudness curve, boosts quiet, monotonic
where:
- `src_vol` = per-source byte = `field32_0x85`. One-shot `volume_byte` param writes it;
  **param=0 means SetVolume is never called → stays at ctor default 0x7f=127 (full).**
  `CExoSoundSource::SetVolume` HARD-CLAMPS to 0x7f. So per-source volume cannot exceed unity.
- `priority_group_volume` = byte copied from `priority_groups[group].volume` at ctor.
  Our 3D cues use group 0 → **vol=106 (NOT 127)** — they run at ~83% of unity, so there's
  headroom. Loaded from `prioritygroups.2da` `Volume` column, **stored as raw byte with NO
  clamp** (only PlaybackVariance clamps to [0,1]) → an Override 2da with Volume>127 stores
  >127 and multiplies in raw → above-unity at the struct level (Miles' final clamp UNVERIFIED).
- `sliderVol` = master SFX slider (`CExoSoundInternal.sound_effect_volume` float), global multiply.

**Priority group ≠ volume lever in isolation.** Group also sets max_player (voice budget),
interrupt, fade, and min/max_volume_distance falloff. Group 0: max_player=4, min_dist=20,
max_dist=10. See [[setsoundmode-priority-group-pause-exemption]] for the pause-exemption role.

**Levers to change cue loudness, in order of certainty:**
1. Attenuate / up-to-unity: pass `volume_byte` 1..127 to PlayCue/PlayCue3D (writes src_vol). CERTAIN.
2. Up to unity from current: put cue on a vol=127 group instead of group 0 (106→127, ~1.2×).
3. Above unity: Override `prioritygroups.2da` custom group Volume>127, OR pre-amplified WAV in
   Override\ keyed by resref. The WAV path is the guaranteed one; the 2da path is plausible but
   needs an in-game listen test (Miles may clamp the float to 1.0).

The runtime probe `probe_priority_groups.cpp` that dumped the live table is
removed (see git history); its dump showed entries 0-26 valid. Relates to [[project_play3doneshotsound_silent_resref]].

---

## play3doneshotsound_silent_resref
_Engine creates no source and produces no audio when a CResRef doesn't resolve in chitin.key — fire-counters can't detect it_

`CExoSound::Play3DOneShotSound` and `PlayOneShotSound` return successfully (no exception, no error indicator) when their `CResRef*` arg points to a string that doesn't resolve through the engine's `Override → streamwaves → streamsounds → streammusic → BIF/RIM` chain. The engine creates no `CExoSoundSourceInternal`, no audio thread work happens, and no sound plays — but a caller-side "play" log line still appears, because only the return code is checked.

**Why:** confirmed 2026-05-07 via per-fire `OnCalculatePitchVarianceFrequency` resref logging. The Wall cue's resref `gui_select` was missing from `chitin.key` for the entire install (only `gui_invselect` exists). 958 `play cue=Wall` log entries → 0 `gui_select` PitchHook fires that session, while every other cue (`fs_metal_droid2`, `gui_quest`, `mgs_s1`, `gui_close`, `gui_invadd`) had matching counts. Replaced with `as_nt_wtrdrip_09`. (The wall cue was removed later with the other exploration sounds; the lesson stands.)

**How to apply:** any time we add or change a cue resref, verify it exists in `chitin.key` (for example with `unkeybif l`) or in a full sound extraction before shipping. Audible silence is the only symptom — the rest of the patch behaves as if the cue fired. The 16-char `CResRef` truncation makes >16-char resrefs silently fail the same way; longest current pick is `as_nt_wtrdrip_09` at exactly 16. The `OnCalculatePitchVarianceFrequency` heartbeat is a permanent canary — if a cue type fires hundreds of times but never appears in `PitchHook` lines, that resref is dead.

---

## cexosoundsource_loop_api
_Full lifecycle API for engine-managed looping/3D sound sources — alternative to fire-and-forget Play3DOneShotSound when continuous tones are needed_

CExoSoundSource is the engine's owned/managed sound source class. Unlike
CExoSound::Play3DOneShotSound (one-shot, no handle), CExoSoundSource is
constructed by the caller and kept alive for the full sound's lifetime,
making it the right primitive for loops, position-updated sources, and
anything that needs Stop().

**Why:** Investigated 2026-05-25 to answer "can we replace the swoop
obstacle 4 Hz ping cadence and the 2 s wall-scrape re-fire with a true
continuous loop?" — yes, the API exists and is fully decompiled.

**How to apply:** When a feature needs sustained spatial audio that
swells/pans naturally over time (a swoop wall scrape, obstacle
approach, ambient zones), reach for CExoSoundSource instead of the
PlayCue3D one-shot wrapper. Lifecycle:

1. `operator new` ~16 bytes (struct is vtable + internal*; 16 is safe)
2. Call ctor: `CExoSoundSource(this)` @ `0x005d5870` — sets vtable and
   allocates the 0xa0-byte internal via the engine's allocator
3. Configure (in any order, before Play):
   - `SetResRef(CResRef*, int)` @ `0x005d61c0`
   - `Set3D(int)` @ `0x005d5910` (1 = positional)
   - `SetPosition(Vector*, float)` @ `0x005d59e0` (float = z offset)
   - `SetPriorityGroup(uint8_t)` @ `0x005d5900`
   - `SetVolume(uint8_t)` @ `0x005d5950`
   - `SetLooping(int)` @ `0x005d59d0` (1 = loop)
4. `Play()` @ `0x005d5930`
5. Per-tick (if moving): `SetPosition(Vector*, 0.0f)`
6. Teardown: `Stop()` @ `0x005d5a20`, then dtor @ `0x005d60a0`, then free

Other methods worth knowing: `GetLooping` @ `0x005d6190`, `SetDistance`
@ `0x005d61a0`, `SetFixedVariance` @ `0x005d5a30`, `SetPitchVariance`
@ `0x005d5980`, `IsHardwarePlayingSoundPleaseDontUseThis` @ `0x005d5920`.

**Canonical engine usage pattern** (decompiled from CSWTrackFollower
::LoadSounds @ `0x0066f7e0` — the swoop bike's own engine-loop loader):
SetSoundName returns a CExoSoundSource* (the bike object owns a slot
array indexed by int). The bike then calls `SetLooping(src, 1)` and
the engine plays it automatically. So the canonical idiom is "configure
+ SetLooping" and the engine takes over.

**Gotcha:** ctor checks the global `DisableSound` flag and leaves
internal=null if true. All methods null-check internal and no-op, so
this is safe but means we should also check GetLooping() or similar
before assuming the source is live.

**In use:** `audio_loop.cpp` wraps this lifecycle as `LoopSource`
(outer struct from CRT `malloc`, which is safe because we never free what
the engine allocated). The swoop race (`minigame_swoop_race.cpp`) and the
audio-glossary audition in Mod settings use it.

Not built yet: one continuous source per in-range swoop obstacle. That
is ~22 concurrent sources on m03mg, so test the voice budget first.

---

## sound_mode_priority_group_pause
_Why a sound is silent under the in-game menu — SetSoundMode(4) pauses all CExoSoundSource except priority groups 1/2/0xb; GUI sounds use 0xb_


The in-game menu silences our cues not via the one-shot SFX path but via a
priority-group-selective pause on the whole sound subsystem.

**Mechanism (decompile-verified 2026-05-31):**
- `CGuiInGame::ShowSWInGameGui` @0x0062c9b0 calls `SetSoundMode(ExoSound, 4)` on open; `HideSWInGameGui` @0x0062cba0 calls `SetSoundMode(ExoSound, 0)` on close.
- `CExoSoundInternal::SetSoundMode` @0x005d5e80 → worker `FUN_005d8560`. Mode 4 (and 2/3) → `PauseAllSounds(0)`; mode 1 → `MuteSound` (global master fade, no exemption); mode 5 → `PauseAllSounds(1)` (pause everything); mode 0 → pop/restore previous mode.
- `PauseAllSounds` @0x005d82c0 with mode 4 pauses every source **except** those whose `priority_group` is **1, 2, or 0xb (11)**.
- `CExoSoundSource` ctor (`CExoSoundSourceInternal` ctor @0x005daf50) defaults `priority_group = 0x17 (23)` → NOT exempt → paused → silent. Volume defaults to 0x7f (full), so volume is never the issue here.
- `CSWGuiManager::LoadGuiSounds` @0x00409f00 builds the engine's own GUI click sounds and calls `SetPriorityGroup(src, 0xb)` — that's why menu clicks stay audible under the pause. `PlayGuiSound` @0x0040a140 plays them through the same `CExoSoundSource::Play` we use.

**How to apply:** Any cue that must be audible while an in-game sub-screen has the world paused must be a directly-driven `CExoSoundSource` with priority group **0xb** set before `Play()` (via `SetPriorityGroup` @0x005d5900, `__thiscall(this, byte)`). The play *path* (`CExoSoundSource::Play`) is fine; the group is the lever. This fixed the audio glossary audition (commit 72b367a) — `LoopSource::Start` gained an optional `priorityGroup` arg, glossary passes 0xb. The title screen worked without it because no paused world / no `SetSoundMode(4)` is active there.

Relates to [[project_messagebox_close_unpause]] (SetSoundMode(0) on popup close), [[project_cexosoundsource_loop_api]] (the source lifecycle API), [[project_play3doneshotsound_silent_resref]].

---

## rolling_footstep_is_vehicle_only
_Confirmed dead-end for HUMANOID footsteps — but the live path for wheeled/floating droid drive loops (see the 2026-08-05 update at the end of this section)_

`CSWCCreature::PlayRollingFootstepSound @0x006107c0`,
`UpdateRollingFootstepSound @0x00610840`, and
`LoadRollingFootstepSound @0x006105b0` are **for vehicles / wheeled units
(swoop bikes, droids on rollers)**, not for the per-step audio of
humanoid characters walking. Despite the name, hooking
`PlayRollingFootstepSound` produces zero handler fires during normal
in-game testing on Endar Spire — verified 2026-05-06 with the byte
probe at `0x006107c0` showing `e9 ?? ?? ?? ?? 90 90 90 90 …` (hook
correctly installed) and the diagnostic "first handler fire" log line
never emitted across full play sessions.

The actual humanoid per-step audio path is **not labeled in Lane's gzf**
and was not identified during the Phase 3 lay-off 5 RE budget
(2026-05-06). Likely candidate: an animation-event callback that calls
`CExoSound::Play3DOneShotSound @0x005d5e10` directly with a
surfacemat-derived `fs_*` resref, but the specific dispatcher hasn't
been located.

`CSWCCreature::DoFootstepAudio @0x0060ded0` is also unrelated — that's
a 5%-probability `as_cv_florcreak{1-3}` ambient floor-creak, not the
regular per-step audio.

**Why:** to keep us from re-hooking a name that looks right but doesn't
fire, and to anchor the next attempt on the chokepoint rather than the
"Rolling" lure.

**How to apply:** when revisiting Phase 3 lay-off 5 (footstep
suppression), don't start from `CSWCCreature::PlayRollingFootstepSound`.
Either (a) instrument `Play3DOneShotSound` with a resref logger to find
the actual caller's EIP, or (b) add a Ghidra xref-dumper script and
trace which of the ~hundreds of callers passes `fs_*` resrefs.

**RESOLVED 2026-05-06.** Path (a) worked: instrumentation hook at
0x005d5e16 logged 2465 fires across mixed walking, frequency-bucket
showed `0x0061a5b6` as THE per-step humanoid caller (203 fires across
7 `fs_metal_*` surface variants). `FindFunction.java` resolved that
EIP to `CSWCCreature::PlayFootstep @0x0061a2d0` — Lane HAD labelled it,
the "Rolling" name was just a lure. Suppression hook landed at 0x0061a31a
(engine's own field6_0x20==0 JZ) with `skip_original_bytes = true`.
See `audio_footstep_suppress.cpp`.

**UPDATE 2026-08-05 — the trio IS the droid drive-loop path.** The
"vehicles / wheeled units" guess above was right, and it matters:
wheeled/floating droids (footstepsounds.2da rows 8/9 — T3-M4 and
astromechs FootstepType=8 `fs_wheel_drdslow`, remotes FootstepType=9
`fs_floatdroid`) get a LOOPING CExoSoundSource at CSWCCreature+0x430
(K2: +0x448), built by `LoadRollingFootstepSound` from the 2DA `rolling`
column (looping, 3D, priority 0x13 for the player creature, `pitchoffset`
cached for the run gait). `UpdateRollingFootstepSound @0x00610840`
(K2 twin 0x0077FB10) drives it per frame purely off
`GetCurrentAnimation`: walk anims 0x2712/0x2713/0x276d/0x2795 → Play
mode 0, run anims 0x2714/0x276e → Play mode 1 (pitch + offset), ANY
other anim → the idle branch Stops the source and clears
playing/mode ([+0x438]/[+0x43c]; K2 [+0x450]/[+0x454], factored into
helper 0x0077FAC0). Because the loop keys off the ANIM, a wall-grinding
droid never goes silent — fixed by `OnUpdateRollingFootstep`
(audio_footstep_suppress.cpp), which consumes into the engine's own
idle/stop branch. Its verdict is STRICTER than the one-shot footstep
verdict: sustained stuck (>=1.2s) plus a forward/backward key to ARM —
instantaneous stuck dips during course corrections and pivot turns must
not cut a continuous loop (first-round lesson, 2026-08-05). The key only
arms; suppression then latches until the stuck state clears, because
releasing it at a wall let the loop blip back for the half second before
the idle animation arrived. K1 hook cut
0x00610843 → idle 0x00610885; K2 cut 0x0077FB19 → idle 0x0077FB73.
It never fired on Endar Spire because no wheeled droid was near — the
2026-05-06 "zero fires" observation, now explained.

---

## party_follower_filter
_filter::ObjectMatches excludes only the PC; anything that announces nearby creatures on its own must skip party followers itself._

`filter::ObjectMatches` excludes only the **leader/PC**
(`GetPlayerServerCreature`), NOT party **followers** — deliberately, so
Q/E can still cycle onto companions. A system that announces creatures
without a key press must skip followers itself, or companions trailing
the player are announced over and over.

**passive_narrate.cpp** does this in `IsActivePartyMember`: it fetches
`GetPartyMembers` and compares server handles (`GetObjectHandle` on the
resolved object). On the automatic focus-follow path a follower is fully
silent; on an explicit Q/E press the name and status still speak.

History: the continuous proximity beacon (`spatial_change_detector.cpp`,
removed with the exploration sounds) needed the same fix on 2026-06-04.
See [[feedback_trust_dev_audio_reads]]. Related party-handle plumbing:
[[project_cserverexoapp_facade_split]].

---

## droid_subtitles_onomatopoeia
_Droid-language subtitles are onomatopoeia not translations; HK-47 speaks Basic; meaning carried by player replies_


Verified from extracted .dlg + binary dialog.tlk on 2026-06-05 against a
German install (lang 2). The quoted lines are the German TLK text; the
English TLK was not re-checked.

**T3-M4 and generic droids speak ONLY onomatopoeia in subtitles** — no
translated/meaningful text exists in their lines. TLK strings literally read
"Biep. Whoop. Wiep. Biep." (ebo_t3m4 strref 39353), "Beep. Beep. Beep."
(tar02_t3m4 22092), "Biep biebp biep-oop!" (k_hdroid1_dialog 38896). The
*meaning* is carried entirely by the PLAYER's reply lines (meaningful prose),
which the mod already narrates from the reply list. So we miss nothing by not
"translating" droidic.

**HK-47 speaks Basic** — full VO + meaningful subtitles (race==DROID but
intelligible).

**Current behaviour:** `IsSuppressibleSpeaker` in dialog_speech.cpp
suppresses the subtitle read-out for race==DROID speakers as well as for
human appearances. So T3's beep text is not read, and HK-47's voiced
Basic line is not doubled by speech. The per-tag lists
(`kNeverSuppressTagsK1`, `kAlwaysSuppressTagsK1`) override the rule for
single speakers.

**Tool:** the check used a local .NET tool (build/dlgdump/, never
committed) that reads chitin.key/BIF, RIM/MOD archives and the binary
dialog.tlk. Companion party convos live in module archives
(ebo_m12aa.mod = Ebon Hawk), NOT in dialog.bif (only 32 DLGs there).

---

