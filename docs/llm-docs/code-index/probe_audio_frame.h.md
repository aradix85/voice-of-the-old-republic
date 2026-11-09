# probe_audio_frame.h (44 lines)

Source: `patches/Accessibility/probe_audio_frame.h`

Audio-frame diagnostic probe.

F10 advances through the 8 compass sectors (N → NE → E → SE → S → SW → W → NW → repeat), emitting a 3D-positional cue at 5m from the player in each direction and speaking the spoken-direction name via Prism. The user listens for whether the audio pan matches the spoken direction.

Purpose: characterise the relationship between our compass frame (used by the beacon description, camera_announce, turn_announce) and the engine's 3D audio listener frame. Pillar 3 beacon testing uncovered a mismatch where the user perceived the heartbeat as coming from a different direction than the description implied. We don't know which axis the engine treats as "listener forward" — this probe lets the user report the actual perceived direction for each compass sector, from which we can build a compensation matrix or confirm there's no mismatch at all.

Cue resref: "gui_open" — distinct from BeaconActive's "gui_check" so the user can audibly separate probe fires from any in-flight beacon.

Hotkey choice: F10 — unbound in stock kotor.ini. Same Win32-polling rationale as the other unbound probes.

## Declarations (in source order)

- L41 `void PollWin32()`: OnUpdate per-tick poll.
