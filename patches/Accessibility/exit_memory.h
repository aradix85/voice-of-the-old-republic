// Exit memory — what lies behind each exit of the player's current region,
// whether the player has been there, and which exit leads back.
//
// The region label ("Junction. Exits: North, Door East, Door South") tells
// the player WHERE the exits are but not WHERE THEY GO. A sighted player
// sees the corridor behind a door and remembers the way they came; this
// module gives the label the same information:
//
//   "Junction. Exits: North, to the Bridge, new; Door East, to a corridor,
//    visited; Door South, back"
//
// Per exit, at speech time:
//   * the region behind it is found by probing the nav-graph regions just
//     past the exit point, away from the current region's centre;
//   * destination: a discovered landmark inside that region ("to the
//     Bridge"), else the region's kind ("to a corridor");
//   * status: "back" when it is the region the player just came from,
//     else "visited" or "new".
// Area-transition doors probe off the walkmesh and stay unannotated (their
// label already names the destination area).
//
// Visited regions are remembered per area for the session (keyed by the
// area resref; region ids are deterministic for a given area build).
//
// Only the player's own region announces are annotated; the view-mode
// cursor keeps the plain label.

#pragma once

#include <string>
#include <vector>

#include "room_topology.h"

namespace acc::exit_memory {

// Area-enter edge (after room_topology::BuildForArea). Switches the visited
// set to this area and forgets the current/previous region.
void OnAreaEntered(void* area);

// The player's committed region changed (transitions' stability-gated
// cluster commit). Records `to` as visited and `from` as the way back.
void OnClusterCommitted(int from, int to);

// Rewrite a player-region label and its exit marks with destination,
// visited and way-back words. False (outputs untouched) when there is
// nothing to add or the label is not in the expected "...: exit, exit"
// shape. On success the exit list is re-joined with "; " so the commas
// inside an annotated exit do not read as extra exits.
bool Annotate(void* area, const std::string& label,
              const std::vector<acc::room_topology::ExitMark>& exits,
              std::string& outLabel,
              std::vector<acc::room_topology::ExitMark>& outExits);

}  // namespace acc::exit_memory
