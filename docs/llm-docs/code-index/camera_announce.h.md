# camera_announce.h (52 lines)

Source: `patches/Accessibility/camera_announce.h`

Camera-direction announce.

KOTOR's default control scheme: A / D rotate the camera around the character (NOT character facing, NOT strafe); W moves the character in the camera's forward direction. Without this announcer the user can't tell where the camera is pointing until they press W and the character snaps to face the camera.

Yaw is derived per tick from the camera and player world positions — the KOTOR orbital camera always looks at the character, so look direction = normalize(player - camera). 8 × 45° compass sectors with 5° hysteresis around the last spoken sector. Urgent SAPI speech routes around NVDA's typed-char cancel since the user is holding A/D.

Tick speaks for exactly one of three reasons, resolved in one place: "stopped" (the turn just ended — immediate, so it doesn't read as lag), "turning" (still rotating — a rate-limited running readout, without which a held turn would be silent until release), and "settled" (the sector has held still long enough to be the final answer). Each covers a gap the others leave; none is redundant.

Because the heading mixes two sources that don't always describe the same moment, a Tab leader swap or an engine camera cut makes it jump rather than turn. Tick classifies those frames as discontinuities and holds all speech until the compass settles, so the player hears the direction they ended on and not the one that briefly appeared on the way.

## Declarations (in source order)

- L32 `void Tick()`
- L36 `bool TryGetCameraEngineYawDegrees(float& out)`: Most recent observed camera yaw in engine frame (0° = +X, CCW positive).
- L49 `bool AnnounceCurrentFacing(unsigned int dedupMs)`: One-shot facing readout for event triggers (e.g.
