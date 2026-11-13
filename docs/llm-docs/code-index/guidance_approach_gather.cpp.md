# guidance_approach_gather.cpp (66 lines)

Source: `patches/Accessibility/guidance_approach_gather.cpp`

A walk that leaves the area waits for the party. The engine refuses an area transition while a companion is not near ("You must gather your party before venturing forth"), and a long walk by Enter outruns the companions. So the leader stops kHoldBeforeM short of the target until everyone is within kGatheredM, and then goes on. After kHoldMaxMs it goes on regardless: a companion that cannot come is the engine's to report.

## Declarations (in source order)

- L28 `bool TickPartyHold(const Vector& pos, DWORD now)`
