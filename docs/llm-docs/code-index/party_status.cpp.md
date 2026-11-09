# party_status.cpp (161 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L45 `std::vector<Member> PartyMembers()`
- L58 `std::string NameOf(void* obj)`
- L64 `bool ReadSecurity(void* creature, int& out)`
- L88 `bool InWorldAndFree()`
- L96 `void PollHotkey()`
- L120 `void Tick()`
- L142 `bool SecurityOf(void* serverCreature, int& outScore)`
- L146 `bool BestSecurity(std::string& outName, int& outScore)`
