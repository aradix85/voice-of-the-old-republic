# party_status.cpp (229 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L51 `std::vector<Member> PartyMembers()`
- L64 `std::string NameOf(void* obj)`
- L70 `bool ReadSecurity(void* creature, int& out)`
- L94 `bool InWorldAndFree()`
- L102 `std::string MembersBehind(float beyondM, float* outLeftM)`
- L121 `std::string HealthLine(bool withLevelUp)`
- L149 `bool AnyoneDown()`
- L156 `void PollHotkey()`
- L165 `void TickHealthWarnings(bool inCombat)`
- L188 `void Tick()`
- L210 `bool SecurityOf(void* serverCreature, int& outScore)`
- L214 `bool BestSecurity(std::string& outName, int& outScore)`
