# guidance_approach_gather.cpp (85 lines)

Source: `patches/Accessibility/guidance_approach_gather.cpp`

A walk that leaves the area waits for the party. The engine refuses an area transition while a companion is not near ("You must gather your party before venturing forth"), and a long walk by Enter outruns the companions. So the leader stops kHoldBeforeM short of the target until everyone is within kGatheredM, and then goes on. It goes on without them, and says so, once nobody behind has come closer for kStillMaxMs (a companion that is stuck, or left behind in solo mode), and after kHoldMaxMs at the latest: a companion that cannot come is the engine's to report.

## Declarations (in source order)

- L30 `void SayAboutParty(acc::strings::Id fmt, const std::string& names, char* line, size_t cap)`
- L38 `bool TickPartyHold(const Vector& pos, DWORD now)`
