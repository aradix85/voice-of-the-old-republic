// The words a region label is made of: octant helpers, exit entries and
// the exit list, door and transition directions, the corridor axis, the
// far-end suffix and ramps.

#include "room_topology_internal.h"

#include "strfmt.h"             // Format — heap-backed printf, never truncates announcements

namespace acc::room_topology {

namespace {

void NoteUnspokenExit(const Vector& pos) {
    if (g_exit_sink) g_exit_sink->push_back({std::string(), pos});
}

}  // namespace

// Direction helpers.

// 8-way octant classifier. Engine frame: +X = East, +Y = North. Returns
// the Id corresponding to the octant the (dx,dy) vector points into.
acc::strings::Id OctantFromVector(float dx, float dy) {
    using acc::strings::Id;
    // atan2 gives radians in (-pi, pi] measured from +X (East) counter-
    // clockwise. Convert to a 0..8 bucket where 0 = East, 1 = Northeast,
    // …, 7 = Southeast.
    float theta = std::atan2(dy, dx);
    // Snap to nearest 45° bucket. Add half-bucket for round-to-nearest.
    constexpr float kPi  = 3.14159265358979323846f;
    constexpr float kStep = kPi / 4.0f;  // 45 degrees
    int bucket = static_cast<int>(std::floor((theta + kStep * 0.5f) / kStep));
    bucket = ((bucket % 8) + 8) % 8;
    switch (bucket) {
        case 0: return Id::DirEast;
        case 1: return Id::DirNortheast;
        case 2: return Id::DirNorth;
        case 3: return Id::DirNorthwest;
        case 4: return Id::DirWest;
        case 5: return Id::DirSouthwest;
        case 6: return Id::DirSouth;
        case 7: return Id::DirSoutheast;
    }
    return Id::DirEast;
}

// Map an Id::Dir* to a stable bit index 0..7 (for dedup masks). Order
// matches OctantFromVector's bucket numbering: E=0, NE=1, N=2, NW=3,
// W=4, SW=5, S=6, SE=7.
int OctantBit(acc::strings::Id dir) {
    using acc::strings::Id;
    switch (dir) {
        case Id::DirEast:      return 0;
        case Id::DirNortheast: return 1;
        case Id::DirNorth:     return 2;
        case Id::DirNorthwest: return 3;
        case Id::DirWest:      return 4;
        case Id::DirSouthwest: return 5;
        case Id::DirSouth:     return 6;
        case Id::DirSoutheast: return 7;
        default: return -1;
    }
}

// Reverse of OctantBit: bit index → direction Id. Used when emitting
// directions in canonical order rather than encounter order.
acc::strings::Id BitToOctant(int bit) {
    using acc::strings::Id;
    switch (bit) {
        case 0: return Id::DirEast;
        case 1: return Id::DirNortheast;
        case 2: return Id::DirNorth;
        case 3: return Id::DirNorthwest;
        case 4: return Id::DirWest;
        case 5: return Id::DirSouthwest;
        case 6: return Id::DirSouth;
        case 7: return Id::DirSoutheast;
    }
    return Id::DirEast;
}

// Spoken form of an attached transition trigger, e.g. "transition East to
// temple exterior". Reuses the door-transition format with the generic
// transition noun (walk-through triggers have no authored loc_name). The
// destination drops the "<planet> - " prefix — every trigger destination
// shares the current planet, so the prefix is pure filler in a label
// that is already the longest entry in its exit list.
std::string RenderTransitionExit(int trigIdx, const Vector& centroid) {
    using acc::strings::Id;
    if (trigIdx < 0 || trigIdx >= g_graph.transition_count) {
        return std::string();
    }
    const TransitionRecord& t = g_graph.transitions[trigIdx];
    Id dirId = OctantFromVector(t.pos.x - centroid.x, t.pos.y - centroid.y);
    const char* dirWord = acc::strings::Get(dirId);
    if (!dirWord[0]) return std::string();

    const char* dest = t.destName;
    if (const char* sep = std::strstr(dest, " - ")) {
        if (sep[3] != '\0') dest = sep + 3;
    }
    const char* noun = acc::strings::Get(Id::CategoryTransition);
    if (!noun[0]) noun = "Übergang";
    const char* fmt = acc::strings::Get(Id::FmtMapCursorDoorTransition);
    if (fmt[0]) return acc::strfmt::Format(fmt, noun, dirWord, dest);
    return acc::strfmt::Format("%s %s %s", noun, dirWord, dest);
}

// Append `entry` to a comma-separated list string, inserting ", " before
// it when the list is non-empty. No-op for empty entries.
void AppendListEntry(std::string& list, const std::string& entry) {
    if (entry.empty()) return;
    if (!list.empty()) list += ", ";
    list += entry;
}

void NoteExit(const std::string& entry, const Vector& pos) {
    if (g_exit_sink && !entry.empty()) g_exit_sink->push_back({entry, pos});
}

// Where an exit entry leads: the door's own position when it is a door,
// else `fallback` (the nav point the shape path measured it from).
Vector ExitPoint(int doorIdx, const Vector& fallback) {
    if (doorIdx >= 0 && doorIdx < g_graph.door_count) {
        return g_graph.doors[doorIdx].pos;
    }
    return fallback;
}

// Render the door-flavoured replacement of a direction word. Single
// source of truth for the dead-end / corridor / junction-octant
// rewrites. Each format takes (noun, direction[, extra]):
//   - noun = door's authored CSWSDoor.loc_name when non-empty (e.g.
//     "Lift", "security door"); falls back to the localized generic
//     "Door"/"Door". Doors named with the generic word collapse to the
//     same output as the fallback, which is fine — no deny-list needed.
// Selection priority for the WHICH format:
//   1. landmarkName non-empty  → "%s %s, %s" (FmtMapCursorDoorLandmark)
//      Used when AttachLandmarksToDoors matched a CSWCWaypoint to this
//      door. Reads cleaner than the area-transition string and lets the
//      proximity-landmark path (transitions::TickProximityLandmarks)
//      suppress a separate redundant announce.
//   2. transitionDest non-empty and != noun → "%s %s to %s" (FmtMapCursorDoorTransition)
//      Engine-derived cross-area destination text (e.g. "Taris -
//      Southern Upper City"). Used when no landmark is attached.
//   3. bare door form → "%s %s" (FmtMapCursorDoor)
std::string RenderDoorDirection(int doorIdx, const char* dirWord) {
    using acc::strings::Id;
    if (!dirWord || !dirWord[0]) return std::string();
    const char* landmark = "";
    const char* dest     = "";
    const char* locName  = "";
    if (doorIdx >= 0 && doorIdx < g_graph.door_count) {
        landmark = g_graph.doors[doorIdx].landmarkName;
        dest     = g_graph.doors[doorIdx].transitionDest;
        locName  = g_graph.doors[doorIdx].locName;
    }
    const char* noun =
        (locName && locName[0]) ? locName
                                : acc::strings::Get(Id::MapCursorDoorNoun);
    if (!noun || !noun[0]) noun = "Door";  // last-ditch fallback if strings table missing

    // Designer-authored doors often duplicate the landmark string into
    // CSWSDoor.loc_name (e.g. Taris Hideout NW exit: loc_name and the
    // companion waypoint's map-note both read "To Apartment Complex").
    // Without this dedup the FmtMapCursorDoorLandmark expansion
    // "%s %s, %s" prints the same word twice — "To Apartment Complex
    // North-West, South, To Apartment Complex". When noun == landmark
    // (case-insensitive), drop the landmark suffix and fall through to
    // the transitionDest or bare-door path.
    bool landmarkEqualsNoun = false;
    if (landmark && landmark[0] && noun && noun[0]) {
        landmarkEqualsNoun = (_stricmp(landmark, noun) == 0);
    }

    if (landmark && landmark[0] && !landmarkEqualsNoun) {
        const char* fmt = acc::strings::Get(Id::FmtMapCursorDoorLandmark);
        if (fmt[0]) return acc::strfmt::Format(fmt, noun, dirWord, landmark);
    }
    // Same duplication on the transition path: a door whose loc_name IS
    // its destination (K2 Residential Module 082 East: "To Entertainment Module 081",
    // K1 Tatooine: "Sand People Enclave") read "X North to X". The
    // name already says where it leads, so drop the suffix.
    bool destEqualsNoun = (dest && dest[0] && _stricmp(dest, noun) == 0);
    if (dest && dest[0] && !destEqualsNoun) {
        const char* fmt = acc::strings::Get(Id::FmtMapCursorDoorTransition);
        if (fmt[0]) return acc::strfmt::Format(fmt, noun, dirWord, dest);
    }
    const char* fmt = acc::strings::Get(Id::FmtMapCursorDoor);
    if (fmt[0]) return acc::strfmt::Format(fmt, noun, dirWord);
    return acc::strfmt::Format("%s %s", noun, dirWord);
}

// Render a corridor's axis label into outBuf. Branches by corridor
// symmetry so the spoken form stays terse for clean axes but exposes
// both endpoints when the corridor turns:
//   - opposite CARDINAL octants (N+S, E+W) AND no door on either end →
//     the dedicated axis words ("North-South" / "East-West"). Unambiguous:
//     neither is an octant name, so they can only be heard as an axis.
//   - opposite DIAGONAL octants (NE+SW, NW+SE) → both direction words,
//     same as the asymmetric form below. These USED to abbreviate to
//     the northern-half octant word ("North-East") — but that word IS a
//     direction name, so by ear it read as "one exit, north-east" and
//     hid the south-west continuation entirely. On the Southern
//     Strand the abbreviated NE–SW connector
//     between the two junctions was the only route to the beach, and
//     its south-west end was never voiced.
//   - non-opposite octants = asymmetric / L-shaped corridor → both
//     direction words rendered, more-northern-first ("corridor West,
//     South-East"). Caught the "corridor West" failure mode where the
//     old single-octant axis collapsed an asymmetric corridor to a
//     single direction and lost the other endpoint.
// `doorA` / `doorB` are the door indices of the A-end and B-end exits
// respectively (-1 = no door). Each end is rendered independently via
// RenderDoorDirection, so a corridor whose ends are different doors
// keeps both names ("security door East, Door West to Fuel Depot")
// instead of collapsing onto whichever door was collected first.
// `deadA` / `deadB` mark an end whose neighbour leads nowhere onward:
// that direction word is wrapped "dead end %s", same as junction
// dead-end octants. Without the marker the Peragus mine shafts droid
// pocket announced as a plain "East" exit and the player searched the
// east wall for a way out that never existed.
// `posA` / `posB` are where each end leads (ExitPoint); when the two
// ends are named separately they are noted as exits (NoteExit). The
// single axis word names the run, not an end, so its ends are noted
// unspoken (NoteUnspokenExit) — the N key still needs where they are.
std::string RenderCorridorAxis(int bitA, int doorA, bool deadA,
                               int bitB, int doorB, bool deadB,
                               bool rampA, bool rampB,
                               const Vector& posA, const Vector& posB) {
    using acc::strings::Id;
    if (bitA < 0 || bitB < 0 || bitA == bitB) return std::string();

    bool anyDoor = (doorA >= 0) || (doorB >= 0);
    bool anyDead = deadA || deadB;
    bool anyRamp = rampA || rampB;

    // The axis collapse names the RUN, not its ends, so it can only be
    // used when neither end is a door. Folding a door into it drops the
    // one fact the player needs — which end the door is on — and the old
    // single-doorIdx form then printed that noun in front of whichever
    // octant happened to sort first: "Door East-West" for a corridor whose
    // east end is a security door and whose west end is a plain Door.
    // Same reasoning that already un-collapsed the diagonal pairs below.
    // A dead-end arm disqualifies the collapse for the same reason a
    // door does: "North-South" names the run and would swallow the one
    // fact that matters — which end is the cul-de-sac. Ditto a ramp end:
    // which end climbs to another level is exactly the fact the collapse
    // would swallow.
    bool cardinalPair = ((bitA ^ bitB) == 4) &&
                        (bitA == 2 || bitB == 2 || bitA == 0 || bitB == 0);
    if (cardinalPair && !anyDoor && !anyDead && !anyRamp) {
        Id wordId = (bitA == 2 || bitB == 2) ? Id::AxisNorthSouth   // N <-> S
                                             : Id::AxisEastWest;    // E <-> W
        const char* word = acc::strings::Get(wordId);
        if (!word[0]) return std::string();
        const char* fmt = acc::strings::Get(Id::FmtMapCursorCorridorDir);
        if (!fmt[0]) return std::string();
        NoteUnspokenExit(posA);
        NoteUnspokenExit(posB);
        return acc::strfmt::Format(fmt, word);
    }

    // Render both endpoints. Order by y-component descending, x-component
    // descending on ties — keeps the more-northern, then more-eastern,
    // end first. Each end's door travels WITH its octant through the
    // swap, so the sort can no longer attach one end's door noun to the
    // other end's direction.
    static const int octant_y[8] = { 0,  1,  2,  1,  0, -1, -2, -1};
    static const int octant_x[8] = { 2,  1,  0, -1, -2, -1,  0,  1};
    int first  = bitA, firstDoor  = doorA;
    int second = bitB, secondDoor = doorB;
    bool firstDead = deadA, secondDead = deadB;
    bool firstRamp = rampA, secondRamp = rampB;
    Vector firstPos = posA, secondPos = posB;
    if (octant_y[bitB] > octant_y[first] ||
        (octant_y[bitB] == octant_y[first] &&
         octant_x[bitB] >  octant_x[first])) {
        first  = bitB; firstDoor  = doorB; firstDead  = deadB;
        firstRamp  = rampB; firstPos  = posB;
        second = bitA; secondDoor = doorA; secondDead = deadA;
        secondRamp = rampA; secondPos = posA;
    }
    // Passable end first: the corridor form "dead end
    // North, South" and the standalone dead-end form "dead end, North"
    // differ only by comma position — near-indistinguishable through a
    // screen reader, with opposite meanings for the marked direction.
    // Keeping the marked end LAST makes the rule parseable by ear: a
    // label that STARTS with "dead end" means you are standing in one;
    // "dead end <dir>" later in the label always names a dead END.
    // Only fires when exactly one end is marked — none or both marked
    // keeps the north/east-first geographic order.
    if (firstDead && !secondDead) {
        int  tb = first;      first      = second;     second     = tb;
        int  td = firstDoor;  firstDoor  = secondDoor; secondDoor = td;
        bool tr = firstRamp;  firstRamp  = secondRamp; secondRamp = tr;
        Vector tp = firstPos; firstPos   = secondPos;  secondPos  = tp;
        firstDead = false;    secondDead = true;
    }
    const char* wordA = acc::strings::Get(BitToOctant(first));
    const char* wordB = acc::strings::Get(BitToOctant(second));
    if (!wordA[0] || !wordB[0]) return std::string();

    // One entry per end, each naming its own door (noun + landmark or
    // transition destination), its dead-end marker, or just its
    // direction. Matches the way ClassifyAsArea already renders
    // per-octant exits, so a two-door corridor keeps both door names
    // instead of losing one. Door wins over the dead-end marker, same
    // rule as the junction emit loop.
    std::string combo;
    std::string firstEntry = (firstDoor >= 0)
                                 ? RenderDoorDirection(firstDoor, wordA)
                                 : DirEntry(BitToOctant(first), firstDead,
                                            !firstDead && firstRamp);
    std::string secondEntry = (secondDoor >= 0)
                                  ? RenderDoorDirection(secondDoor, wordB)
                                  : DirEntry(BitToOctant(second), secondDead,
                                             !secondDead && secondRamp);
    AppendListEntry(combo, firstEntry);
    AppendListEntry(combo, secondEntry);
    if (combo.empty()) return std::string();
    NoteExit(firstEntry, firstPos);
    NoteExit(secondEntry, secondPos);

    if (anyDoor) return combo;
    const char* fmt = acc::strings::Get(Id::FmtMapCursorCorridorDir);
    if (fmt[0]) return acc::strfmt::Format(fmt, combo.c_str());
    return std::string();
}

// Render one direction entry. When `markDeadEnd` is set, the direction
// word is wrapped via FmtMapCursorJunctionDeadEndExit ("dead end %s") so
// the user knows that exit doesn't lead onward (junctions whose edges
// include degree-1 stubs call them out). Prefix form mirrors the door
// rewrite ("Door %s to %s") so every special exit reads NOUN-then-
// direction within the junction list.
std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd,
                     bool markRamp) {
    const char* word = acc::strings::Get(dirId);
    if (!word[0]) return std::string();
    if (markDeadEnd) {
        const char* fmt = acc::strings::Get(
            acc::strings::Id::FmtMapCursorJunctionDeadEndExit);
        if (fmt[0]) return acc::strfmt::Format(fmt, word);
    }
    // Ramp annotation ("Ramp %s") — only when the dead-end marker
    // didn't already claim the entry: a dead end isn't worth the walk
    // whether it ramps or not, so the dead-end verdict is the decision-
    // relevant one. Door rewrites never reach DirEntry at all.
    if (markRamp) {
        const char* fmt = acc::strings::Get(
            acc::strings::Id::FmtRampExit);
        if (fmt[0]) return acc::strfmt::Format(fmt, word);
    }
    return std::string(word);
}

// Classify a cluster (1 or more nodes) by its centroid and the list of
// external neighbour nodes (nodes outside this cluster that share an
// edge with some member). Renders the label + kind + sig into the
// out-params.
//
// areaHint: 0 = not an area (use the graph deadend/corridor/junction
// rendering below); 1 = room (immediate-announce); 2 = big area
// (delayed-announce). Non-zero short-circuits to the unified neutral
// "Area [axis]. exits: …" rendering — open spaces, merged rooms
// and merged big junctions all read the same way, geometry = long axis
// only when elongated, then the exits. centroidFloorZ feeds the axis
// probe.
//
// The graph path (areaHint == 0) runs for any cluster the caller did NOT
// flag as a probe-owned place: every singleton, plus multi-node hubs that
// merged by the space rule but carry no open/room geometry (a merged
// corridor-junction). It renders dead end / corridor / junction from the
// exit topology, never the merged-space "plaza". For a multi-node hub the
// centroid is the synthesized member midpoint and externalNbs is the union
// of every member's external exits, so the same per-octant logic applies:
//   externalCount == 1  → dead-end (direction = centroid → that exit)
//   externalCount == 2  → corridor (axis between the two exits,
//                                   cardinal cases use "North-South" /
//                                   "East-West" axis words)
//   externalCount >= 3  → junction ("junction").
//                         8-octant bucketing with passable-wins:
//                         per-octant aggregation marks the octant as
//                         dead-end only when EVERY exit in it leads
//                         to a degree-1 neighbour. Any passable exit
//                         in the octant suppresses the marker — fixes
//                         the order-dependent first-wins bug from the
//                         initial revision.
// trigIdxs/trigCount: indices into g_graph.transitions attached to this
// cluster (nearest nav node is a member). Each renders as a named exit
// entry appended to the label — and a dead-end cluster that carries one
// is no dead end at all: the walkmesh ends there because the AREA ends
// there, so it re-renders as a passage toward the transition instead of
// "dead end" (the Southern Strand exit-corridor fix).
// ---------------------------------------------------------------------
// ClassifyCluster's shape branches.
//
// ClassifyCluster picks exactly one of these and returns; each writes the
// label and kind for the shape it names and nothing else. They keep their
// original bodies verbatim, so a `return` inside one still means what it
// meant inline: this cluster is classified, stop.
//
// The parameter lists are long, but each one is now a statement about
// what that shape actually depends on — the corridor branch never looks
// at areaHint or the trigger count, the demoted branches never look at
// the nav graph at all.
// ---------------------------------------------------------------------

// Far-end exit qualifier (see the FarAt* string ids). An exit's octant
// word is its bearing from the zone centre; octant rounding swallows up
// to 22.5°, which across a zone-scale footprint hides that the exit
// sits at one END of the zone. Returns the suffix ("far to the
// north") when the exit lies in the outer band of a long-enough axis
// AND the announced word lacks that end's cardinal component — the
// component check is what keeps expected corner exits ("South-West" exit
// in the far south-west) unqualified. At most one axis can ever
// qualify: the word's own components are suppressed, and the bearing
// guarantees the word contains the dominant axis. nullptr = no
// qualifier (including every not-sure case — silence over speculation).
const char* FarEndSuffix(int bit, const Vector& exitPos,
                         const float* bboxXY) {
    using acc::strings::Id;
    if (!bboxXY || bit < 0 || bit > 7) return nullptr;
    constexpr float kFarAxisMinM   = 20.0f;
    constexpr float kFarBandFrac   = 0.6f;
    // Octant → axis components, index order E,NE,N,NW,W,SW,S,SE.
    static const int kOctY[8] = { 0,  1,  2,  1,  0, -1, -2, -1};
    static const int kOctX[8] = { 2,  1,  0, -1, -2, -1,  0,  1};

    float lenX = bboxXY[2] - bboxXY[0];
    float lenY = bboxXY[3] - bboxXY[1];
    float cx   = 0.5f * (bboxXY[0] + bboxXY[2]);
    float cy   = 0.5f * (bboxXY[1] + bboxXY[3]);

    Id    bestId  = Id::FarAtNorth;
    float bestF   = 0.0f;
    if (lenY >= kFarAxisMinM) {
        float fy = (exitPos.y - cy) / (0.5f * lenY);
        if (fy >= kFarBandFrac && kOctY[bit] <= 0 && fy > bestF) {
            bestF = fy; bestId = Id::FarAtNorth;
        }
        if (-fy >= kFarBandFrac && kOctY[bit] >= 0 && -fy > bestF) {
            bestF = -fy; bestId = Id::FarAtSouth;
        }
    }
    if (lenX >= kFarAxisMinM) {
        float fx = (exitPos.x - cx) / (0.5f * lenX);
        if (fx >= kFarBandFrac && kOctX[bit] <= 0 && fx > bestF) {
            bestF = fx; bestId = Id::FarAtEast;
        }
        if (-fx >= kFarBandFrac && kOctX[bit] >= 0 && -fx > bestF) {
            bestF = -fx; bestId = Id::FarAtWest;
        }
    }
    if (bestF <= 0.0f) return nullptr;
    const char* sfx = acc::strings::Get(bestId);
    return sfx[0] ? sfx : nullptr;
}

// Does the exit toward external neighbour `nb` cross a floor-level
// change? True when the neighbour's calibrated floor height differs
// from the cluster's mean floor by at least the same-floor tolerance —
// the identical threshold the wall filter uses to decide "different
// storey", so an exit is a ramp exactly when its far side would be
// z-vetoed from here. Reads g_graph.node_floor_z, persisted before
// Pass 3 runs. Intentionally conservative: a neighbour node sitting
// mid-ramp can under-read the delta and stay unannotated — the user
// asked for no spam, so borderline slopes stay plain directions.
bool ExitIsRamp(int nb, float clusterFloorZ) {
    float dz = g_graph.node_floor_z[nb] - clusterFloorZ;
    if (dz < 0.0f) dz = -dz;
    return dz >= acc::engine::kWallCrossZToleranceM;
}

}  // namespace acc::room_topology
