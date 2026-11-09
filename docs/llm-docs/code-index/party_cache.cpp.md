# party_cache.cpp (109 lines)

Source: `patches/Accessibility/party_cache.cpp`

(From party_cache.h.) Party-member-name cache.

Used by the combat msg-bus filter to decide whether an incoming "X attacks Y" line targets a party member (PC or active follower). Snapshots names from the engine's display-name accessor — same bytes the engine puts into the message buffer — so comparison is locale- clean without a separate map for party names.

Lazy refresh on IsPartyMember if the cache is older than the backstop (1s). Roster changes only on recruit/swap/dismiss — rare and manual.

## Declarations (in source order)

- L24 `void Refresh()`
- L90 `void RefreshIfStale()`
- L99 `bool IsPartyMember(const char* name)`
