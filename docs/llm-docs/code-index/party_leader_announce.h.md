# party_leader_announce.h (18 lines)

Source: `patches/Accessibility/party_leader_announce.h`

Tab leader-change announce.

Speaks the new controlled creature's name on Tab rising-edge. Repetition is intentional — in solo mode Tab on the same creature still speaks, confirming "still solo" rather than swallowing the press.

Fires in panels too — engine strip panels re-bind to the new leader, so the announce is wanted in both world and UI contexts.

## Declarations (in source order)

- L15 `void Tick()`: Foreground + player-loaded gates inside.
