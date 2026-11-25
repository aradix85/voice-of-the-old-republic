# party_status.cpp (251 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L59 `std::vector<Member> PartyMembers()`
- L72 `std::string NameOf(void* obj)`
- L78 `bool ReadSecurityK2(void* creature, int& out)`
- L92 `bool ReadSecurity(void* creature, int& out)`
- L116 `bool InWorldAndFree()`
- L124 `std::string MembersBehind(float beyondM, float* outLeftM)`
- L143 `std::string HealthLine(bool withLevelUp)`
- L171 `bool AnyoneDown()`
- L178 `void PollHotkey()`
- L187 `void TickHealthWarnings(bool inCombat)`
- L210 `void Tick()`
- L232 `bool SecurityOf(void* serverCreature, int& outScore)`
- L236 `bool BestSecurity(std::string& outName, int& outScore)`
