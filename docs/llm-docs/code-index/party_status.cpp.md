# party_status.cpp (183 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L45 `std::vector<Member> PartyMembers()`
- L58 `std::string NameOf(void* obj)`
- L64 `bool ReadSecurity(void* creature, int& out)`
- L88 `bool InWorldAndFree()`
- L96 `std::string MembersBehind(float beyondM)`
- L113 `std::string HealthLine(bool withLevelUp)`
- L133 `void PollHotkey()`
- L142 `void Tick()`
- L164 `bool SecurityOf(void* serverCreature, int& outScore)`
- L168 `bool BestSecurity(std::string& outName, int& outScore)`
