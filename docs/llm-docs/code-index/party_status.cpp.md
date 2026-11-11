# party_status.cpp (160 lines)

Source: `patches/Accessibility/party_status.cpp`

Party status — see party_status.h.

## Declarations (in source order)

- L44 `std::vector<Member> PartyMembers()`
- L57 `std::string NameOf(void* obj)`
- L63 `bool ReadSecurity(void* creature, int& out)`
- L87 `bool InWorldAndFree()`
- L95 `void PollHotkey()`
- L119 `void Tick()`
- L141 `bool SecurityOf(void* serverCreature, int& outScore)`
- L145 `bool BestSecurity(std::string& outName, int& outScore)`
