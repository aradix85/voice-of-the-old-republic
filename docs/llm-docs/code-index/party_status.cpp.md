# party_status.cpp (270 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L64 `std::vector<Member> PartyMembers()`
- L77 `std::string NameOf(void* obj)`
- L83 `bool ReadSecurityK2(void* creature, int& out)`
- L97 `bool ReadSecurity(void* creature, int& out)`
- L121 `bool InWorldAndFree()`
- L129 `std::string MembersBehind(float beyondM, float* outLeftM)`
- L148 `std::string HealthLine(bool forKey)`
- L190 `bool AnyoneDown()`
- L197 `void PollHotkey()`
- L206 `void TickHealthWarnings(bool inCombat)`
- L229 `void Tick()`
- L251 `bool SecurityOf(void* serverCreature, int& outScore)`
- L255 `bool BestSecurity(std::string& outName, int& outScore)`
