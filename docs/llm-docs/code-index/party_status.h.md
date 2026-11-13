# party_status.h (54 lines)

Source: `patches/Accessibility/party_status.h`

Party status — everyone's health on one key, a notice when someone can level up, and who in the party is best at Security.

H speaks only the controlled leader. A sighted player sees every party portrait's health bar and the level-up arrow on the portrait at a glance; this module gives the same overview:

Ctrl+H  -> "Korasha 10 of 13, can level up; Trask 27 of 36"

and, once per member each time it becomes true (polled about once a second, held back during combat, dialogue and screens):

"Korasha can level up"

Without that notice a player can walk into "You must level up before you pass through the door" with no idea why the door stays shut.

BestSecurity feeds the lock detail ("Security 15, best in your party: Trask with 4"). KOTOR 1 only: skill rank + Wisdom modifier read from CSWSCreatureStats (1.03 address DB), the skill's key ability in skills.2da. Item bonuses are not counted.

## Declarations (in source order)

- L30 `void PollHotkey()`: Ctrl+H (Action::PartyStatusAnnounce).
- L34 `std::string HealthLine(bool withLevelUp)`: "Korasha 10 of 13; Trask 27 of 36", with ", can level up" after a member when asked for.
- L39 `std::string MembersBehind(float beyondM, float* outLeftM = nullptr)`: The companions farther than `beyondM` from the controlled character, by name ("Mission, Zaalbar").
- L42 `void Tick()`: Once per frame (core_tick): the level-up notice.
- L47 `bool BestSecurity(std::string& outName, int& outScore)`: The party member (player included) with the highest Security, name in the game codepage.
- L51 `bool SecurityOf(void* serverCreature, int& outScore)`: One creature's Security as BestSecurity counts it (KOTOR 1).
