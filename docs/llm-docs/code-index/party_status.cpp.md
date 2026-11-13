# party_status.cpp (185 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L45 `std::vector<Member> PartyMembers()`
- L58 `std::string NameOf(void* obj)`
- L64 `bool ReadSecurity(void* creature, int& out)`
- L88 `bool InWorldAndFree()`
- L96 `std::string MembersBehind(float beyondM, float* outLeftM)`
- L115 `std::string HealthLine(bool withLevelUp)`
- L135 `void PollHotkey()`
- L144 `void Tick()`
- L166 `bool SecurityOf(void* serverCreature, int& outScore)`
- L170 `bool BestSecurity(std::string& outName, int& outScore)`
