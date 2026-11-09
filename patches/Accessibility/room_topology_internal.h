// Internal to the room_topology module (room_topology*.cpp); no other file includes
// it. The public interface is room_topology.h. Here: what the files share.

#pragma once

#include "room_topology.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "engine_area.h"        // AreaObjectIterator, GetObjectKind, GameObjectKind::Door, GetObjectPosition, kDoorTransitionDestOffset, SegmentCrossesWalkmesh
#include "engine_navgraph.h"
#include "wall_probe.h"         // the wall system this module reads
#include "strings.h"

namespace acc::room_topology {

// The walkmesh probes of wall_probe ("is there a wall, how far"), which this
// room-shape system consumes.
using acc::wall_probe::ProbeDistance;
using acc::wall_probe::IsAlcoveAlongAxis;
using acc::wall_probe::ProbeClearance8;

// Tunable parameters.
// Re-use the navgraph cap so per-node arrays line up with the snapshot.
constexpr int kMaxNodes = acc::engine::navgraph::kMaxNodes;

// Clustering runs in two passes (see BuildForArea). The old six merge
// passes were two operations wearing six coats, so they fold to:
//   Pass 1 — core merge: ONE nav-edge walk that unions a clear, door-free
//            edge whose endpoints are the same kind. Two rules: space
//            (both are a place the player stands in — degree-≥3 hub or
//            open/room by probe) and corridor (both straight degree-2).
//            Space↔passage never merge. The old junction / room / open
//            rules collapsed into the single space rule — separating them
//            only left door-less interiors fragmented at each class seam.
//   Pass 2 — straggler absorb: fold a degree-≤2 corner/spoke that matched
//            no core rule into an adjacent core (graph-adjacent, or bbox-
//            contained for graph-unattached nodes). The old hub + bbox
//            absorption passes.
// The constants below are the gates, tagged by the rule that uses them.
//
// Pass 1 / space rule — vertical gate. The planar distance gate is the
// unified space cap (kOpenMergeRadiusM, applied as kSpaceCapSq in the
// Pass 1 body); this Z gate stops multi-floor stacks (Sith base, Endar
// Spire decks) from collapsing vertically-aligned nodes. The old separate
// 8m junction cap is gone — merging two degree-≥3 hubs is just one case of
// the space rule now (this still collapses the Dias-Apartment pattern of
// two Kreuzungen ~6m apart into one perceptual hub).
constexpr float kMergeMaxZM = 1.0f;

// Pass 1 / open rule — the large-space analog of the corridor and room
// rules: it folds the several nav nodes of one open plaza/chamber into a
// single cluster, so the player hears one place instead of flipping
// between adjacent Kreuzungen (the Slums central-plaza re-fire — clusters
// 110/118 swap on every short loop).
//
// A node is "open" when at least kOpenMinRays of its 8 clearance rays
// (cast at the calibrated floor height — see the clearance block) clear
// kOpenRayDistM. Two open nodes that are GRAPH-ADJACENT (share a nav
// edge), within kOpenMergeRadiusM, mutually wall-clear and door-free
// union into one open cluster. Adjacency is the connector — proximity
// alone bridged junction throats (the Oberstadt store/cantina over-merge);
// the engine's edges define what is actually contiguous. See the Pass 1
// body for the full rationale + Slums adjacency audit.
//
// Threshold rationale (measured on the Slums and Manaan Östliches
// Zentrum): a pure corridor X-junction has at most as many long rays as
// it has arms, so its over-8m count tops out at ~3-5; a genuinely open
// room needs its surrounding walls pulled back and reaches 6-8. Setting
// kOpenMinRays=6 admits real rooms and rejects corridor intersections by
// construction — confirmed both directions on real content (Slums
// courtyard junctions 6-8 → merge; Manaan corridor junctions 3-5 →
// stay). The boundary lives at 5 (≈7m spaces) — nuisance, not safety.
//
// kOpenMergeRadiusM=16m is now a secondary sanity cap on the length of an
// open→open graph edge we'll coalesce across (adjacency is the primary
// connector). It covers every real Slums plaza edge (the largest open
// open-coalesce gap there is 16.0m, node 110↔118 is 15.3m and is a direct
// nav edge) while leaving long cross-plaza edges (e.g. Oberstadt 11↔18 at
// 23.7m) as separate spaces — current behaviour. The earlier "raise 12→16
// to un-strand node 110" tuning is now moot: 110 was never stranded in the
// graph, only in the old proximity form. The archway residual (two open
// rooms joined by a wide door-less opening merging across it) is now gated
// by adjacency too — they only merge if the engine authored an edge across
// the opening. Tune the cap from harvested cluster sizes if a real plaza
// turns out to need a longer adjacent edge.
constexpr float kOpenRayDistM     = 8.0f;
constexpr int   kOpenMinRays      = 6;
constexpr float kOpenMergeRadiusM = 16.0f;

// Large-area threshold: a cluster whose member-node bounding box has its
// longer side ≥ this many metres speaks as "Großer Bereich" instead of
// "Bereich". Pure label swap — axis, exits, kind and delay are unchanged;
// it just sets the expectation that wall/object cues are sparse across a
// space this big (the Kashyyyk Great Walkway's main cluster spans ~160 m,
// normal merged rooms ~20-30 m). Tuned conservatively so only genuinely
// large open spaces trip it; raise/lower after in-game feedback.
constexpr float kLargeAreaExtentMeters = 40.0f;

// Kind values now live in the public header (room_topology.h) so
// transitions.cpp can branch on Platz for the delayed-announce path.
// Aliases here keep the local code compact.
constexpr int kKindDeadEnd  = KindDeadEnd;
constexpr int kKindCorridor = KindCorridor;
constexpr int kKindJunction = KindJunction;
constexpr int kKindOpenArea = KindOpenArea;
constexpr int kKindPlatz    = KindPlatz;
constexpr int kKindRoom     = KindRoom;

// Cached state for the current area.
// Per-area door snapshot. Captured on BuildForArea (alongside the
// nav-graph snapshot) so the per-edge "is there a door between these
// two nav points?" query doesn't have to re-iterate game_objects[] +
// SEH-read every time. Transition destination is the CExoLocString at
// CSWSDoor +0x3c8 — non-empty when this door is an area-transition
// trigger (cross-area door), empty for normal in-area doors.
struct DoorRecord {
    Vector pos;
    char   transitionDest[64];
    // CSWSDoor.loc_name @+0x39c — the door's own authored localized
    // name (e.g. "Lift", "Sicherheitstür", "Tür zu Bastilas Quartier").
    // Substituted as the noun in the FmtMapCursorDoor* formats; when
    // empty, the localized generic ("Tür"/"Door") falls back so output
    // matches the pre-loc-name behaviour for unnamed doors. Same field
    // Pillar 4 cycle narration speaks when Q/E lands on a door.
    char   locName[64];
    // Attached landmark map-note (e.g. "Zur Oberstadt"), populated by
    // AttachLandmarksToDoors during BuildForArea when a CSWCWaypoint
    // with has_map_note sits within kLandmarkDoorMatchMaxM of this
    // door's position. Preferred over transitionDest in cluster labels
    // (see RenderDoorDirection) — the landmark name is the content
    // author's canonical phrasing and reads cleaner than the raw
    // area-transition string. Empty when no landmark matched.
    char   landmarkName[64];
};

constexpr int kMaxDoors = 128;

// Walk-through area-transition triggers (CSWSTrigger with a non-empty
// transition destination). Outdoor maps (Lehon beaches) use these instead
// of doors for area exits: the walkmesh simply ends inside the trigger
// polygon, so the nav graph sees a dead end there and the cluster label
// said "Sackgasse" at the very spot that leaves the area (the Südlicher
// Strand's only walk-through exit). Snapshotted once per build like doors;
// each record
// binds to its nearest nav node so Pass 3 can voice the transition as a
// named exit of that node's cluster.
struct TransitionRecord {
    Vector pos;
    char   destName[64];   // resolved TransitionDestin ("Unbekannte Welt -
                           // Tempel-Außenbereich"); spoken with the
                           // "<planet> - " prefix stripped
    int    nearestNode;    // -1 when no nav node within attach range
};

constexpr int   kMaxTransitions       = 16;
// Nearest-node attach cap. The trigger sits past the walkmesh edge, so it
// is farther from its nav node than a door is from its frame (Südlicher
// Strand temple trigger: 7.3 m). Beyond this we leave the trigger
// unattached and log it rather than guess across half the map.
constexpr float kTransitionAttachMaxM = 12.0f;

struct AreaGraph {
    void*       area_owner   = nullptr;
    bool        built        = false;
    int         node_count   = 0;
    Vector      node_pos    [kMaxNodes];
    std::string node_label  [kMaxNodes];
    // The label's exit entries with their world points (ExitMark), written
    // with node_label by Pass 3. Feeds exit_memory and camera_orient.
    std::vector<ExitMark> node_exits[kMaxNodes];
    int         node_sig    [kMaxNodes];
    int         node_kind   [kMaxNodes];  // see kKind* constants
    // Frozen UFFind root per node, snapshotted at the end of BuildForArea
    // after all merge passes complete. Used as the perceptual-region
    // trigger key by transitions::Tick — walking inside one cluster
    // keeps the id constant, so cluster-change is a clean signal for
    // "the player just entered a different region".
    int         node_cluster_id[kMaxNodes];
    // Per-node "filtered" flag. Set to true on degree-1 clusters whose
    // walkmesh-shape gate rejected the alcove claim. The label is still
    // rendered (Sackgasse + direction) so the LookupAt rescue path can
    // emit it when no unfiltered candidate sits within range — recovers
    // the rare Bucket-3 case (alcove dropped by the geometry gate, no
    // labelled cluster nearby → would otherwise speak the neutral "Bereich").
    // The primary scan in LookupAt skips filtered nodes, so wall-curve
    // graph artefacts in inhabited regions stay silent (the core-merge and
    // straggler-absorb passes also absorb most such artefacts before the gate
    // runs; surviving filtered singletons are graph-isolated and almost
    // always genuine alcoves).
    bool        node_filtered  [kMaxNodes];
    // Longest bounding-box side of the node's cluster, written alongside
    // label/kind in Pass 3. Consumed by GetClusterInfo so transitions.cpp
    // can dwell-gate sub-perceptual clusters (extent below a floor) at
    // announce time without touching the clustering itself.
    float       node_extent    [kMaxNodes];
    // Centroid of the node's cluster — the point every exit word in its
    // label is measured from. Consumed by GetClusterInfo so transitions.cpp
    // can time a label to the player nearing that point (the centre rule).
    Vector      node_centre    [kMaxNodes];
    // ClusterInfo::footprintM of the node's cluster, written with it.
    float       node_footprint [kMaxNodes];
    // Calibrated walkmesh floor height per node (the clearance probe's
    // probeZ, see ComputeNodeShapeFeatures). Engine path nodes carry
    // z=0; this is the resolved real height, persisted so LookupAt's
    // zTrusted filters can compare the player's height against the
    // node's actual floor instead of the meaningless stored zero.
    float       node_floor_z   [kMaxNodes];
    int         door_count   = 0;
    DoorRecord  doors       [kMaxDoors];
    int              transition_count = 0;
    TransitionRecord transitions[kMaxTransitions];
    // Region adjacency (ClusterLink), recorded at the end of BuildForArea
    // for the exploration overview. Not used by labelling.
    std::vector<ClusterLink> cluster_links;
};

extern AreaGraph g_graph;

struct DoorStabilityState {
    bool committed   = false;
    int  last_count  = -1;
    int  streak      = 0;
    int  retry_ticks = 0;
};
extern DoorStabilityState g_doors_stability;

// Unified edge classifier — Clear / Door / Blocked.
// Edge verdict for a nav-graph segment AB. Drives both the Pass 1
// core-merge / Pass 2 absorb merge-veto gates and external-neighbour
// filtering in Pass 3 classification:
//   - kEdgeClear   → free movement; contributes its direction unmodified
//   - kEdgeDoor    → free movement through a CSWSDoor (doorIdx valid);
//                    contributes direction wrapped as "Tür X" / "Tür X
//                    nach DEST"
//   - kEdgeBlocked → the segment crosses immovable walkmesh geometry;
//                    the edge is dropped entirely (no merge, no
//                    external-edge contribution).
enum EdgeClass {
    kEdgeClear   = 0,
    kEdgeDoor    = 1,
    kEdgeBlocked = 2,
};

struct EdgeResult {
    EdgeClass kind;
    int       doorIdx;  // valid only when kind == kEdgeDoor
};

// Tallies for the end-of-build summary. Counts are multi-fire across
// passes (an edge classified by Pass 1 and again by Pass 2 contributes
// twice); the summary log line documents this. Reset at BuildForArea
// entry; we never read them outside the same BuildForArea call.
extern int s_class_clear;
extern int s_class_door;
extern int s_class_blocked;
extern int s_caveat1_hits;  // door inside walkmesh wall
extern int s_caveat2_hits;  // walk-clear edge between distinct .wok rooms

// Exit-mark sink (see ExitMark). PassClassifyClusters points it
// at the cluster's mark list around ClassifyCluster; every place that
// appends an EXIT entry to a label notes the entry and its world point
// here. Null outside classification, so the label helpers stay usable
// on their own. An exit the label names only through its axis word is
// noted with empty text (NoteUnspokenExit): SpeakExitList skips it, the
// N key (camera_orient) still turns to its point.
extern std::vector<ExitMark>* g_exit_sink;

// Union-find over node ids. Used to merge directly-connected junctions
// into single perceptual clusters. Reused for every BuildForArea
// invocation (reset at the start of the function).
extern int s_uf_parent[kMaxNodes];

// The working set BuildForArea's passes hand to each other. It lives on
// BuildForArea's stack for the duration of one build and is threaded
// through the passes by reference — the passes own no state of their own,
// which is what lets them be read (and reordered, and tested) one at a
// time.
struct BuildPasses {
    // Per-node shape features. Computed once before Pass 1 and read by
    // every pass: is this node "space" (a place the player stands in) or
    // "passage" (a link between places), and at what floor height.
    bool  nodeOpen  [kMaxNodes];
    bool  nodeRoom  [kMaxNodes];
    float nodeFloorZ[kMaxNodes];
    // Walkmesh dead-end verdict for degree-1 nodes (false for all
    // others). Read by the pendant absorb — which un-clusters regions
    // the walkmesh says don't terminate — and by every classification
    // site that used to re-probe this for itself.
    bool  nodeDeadEnd[kMaxNodes];
    // Per-node axis extents from the same 8-ray probe (E-W, NE-SW, N-S,
    // NW-SE). Pass 3 takes the per-cluster median so the elongation
    // verdict comes from the whole cluster, not one centroid probe.
    float nodeAxisExt[kMaxNodes][4];
    // Corridor straightness. Precomputed so Pass 1's corridor rule is a
    // both-straight test in the edge-walk; also read by Pass 1b (to pick
    // the straightest neighbour to fold a bend into) and by Pass 2 (to
    // keep corridor material out of the straggler absorb).
    bool  chainStraight   [kMaxNodes];
    float chainStraightCos[kMaxNodes];
    // Cached walls for the edge-clearness test, shared by Passes 1 and 2.
    const acc::engine::WallEdge* gw = nullptr;
    int   gwCount = 0;
    bool  haveGW  = false;
    // The counters the end-of-build summary reports. Each pass logs its
    // own veto breakdown locally from its own locals; only these ten
    // cross a pass boundary.
    int coreSpace = 0, coreCorridor = 0;
    int absorbAdj = 0, bboxAbsorbed = 0;
    int clusters = 0, multiNodeClusters = 0;
    int deadEnds = 0, corridors = 0, junctions = 0, openAreas = 0;
};

// Helpers shared across the files, by the file that defines them.

// room_topology_doors.cpp
void SnapshotDoors(void* area);
void SnapshotTransitionTriggers(void* area);
int FindDoorOnEdge(const Vector& a, const Vector& b);
EdgeResult ClassifyEdge(void* areaForDiag, const Vector& a, const Vector& b, const char* callerCtx);

// room_topology_merge.cpp
int UFFind(int x);
void UFUnite(int a, int b);
void ComputeChainStraightness(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);
void PassCoreMerge(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);
void PassCornerFold(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);

// room_topology_absorb.cpp
void AbsorbAdjacentStragglers(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);
void AbsorbStragglersByBoundingBox(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);
void AbsorbOpenEndedPendants(const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);

// room_topology_shape.cpp
int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node);
void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4]);
bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId);
int AxisOctantMask(acc::strings::Id axisId);
void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp);

// room_topology_words.cpp
acc::strings::Id OctantFromVector(float dx, float dy);
int OctantBit(acc::strings::Id dir);
acc::strings::Id BitToOctant(int bit);
std::string RenderTransitionExit(int trigIdx, const Vector& centroid);
void AppendListEntry(std::string& list, const std::string& entry);
void NoteExit(const std::string& entry, const Vector& pos);
Vector ExitPoint(int doorIdx, const Vector& fallback);
std::string RenderDoorDirection(int doorIdx, const char* dirWord);
std::string RenderCorridorAxis(int bitA, int doorA, bool deadA, int bitB, int doorB, bool deadB, bool rampA, bool rampB, const Vector& posA, const Vector& posB);
std::string DirEntry(acc::strings::Id dirId, bool markDeadEnd, bool markRamp = false);
const char* FarEndSuffix(int bit, const Vector& exitPos, const float* bboxXY);
bool ExitIsRamp(int nb, float clusterFloorZ);

// room_topology_label_kinds.cpp
void ClassifyAsArea(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int areaHint, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig);
void ClassifyAsDeadEnd(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, int trigCount, bool gateAgrees, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered);
void ClassifyAsCorridor(const acc::engine::navgraph::NavGraphSnapshot& g, int n, const Vector& centroid, float centroidFloorZ, const int* externalNbs, const int* externalDoorIdx, const bool* nodeDeadEnd, const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig);
void ClassifyDemotedToDeadEnd(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig);
void ClassifyDemotedToCorridor(const Vector& centroid, const int octantDoorIdx[8], const int realExitBits[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig);
void EmitJunctionLabel(const bool octantHasExit[8], const bool octantAllDeadEnd[8], const int octantDoorIdx[8], const bool octantAllRamp[8], const Vector octantPos[8], const std::string& trigEntries, std::string& outLabel, int& outKind, int& outSig);

// room_topology_label.cpp
void ClassifyCluster(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, const int* trigIdxs, int trigCount, int areaHint, float centroidFloorZ, bool isLargeArea, const float* bboxXY, const float* axisExt, const bool* nodeDeadEnd, bool terminalDeadEndAgrees, std::string& outLabel, int& outKind, int& outSig, bool& outFiltered);

// room_topology_diag.cpp
void LogDoorSnapshotDetails(void* area);
void LogNavWallCrossings(const acc::engine::navgraph::NavGraphSnapshot& g);
void LogTopologyMetrics(const acc::engine::navgraph::NavGraphSnapshot& g, int n);
void LogClusterMemberAdjacency(const acc::engine::navgraph::NavGraphSnapshot& g, int n);

}  // namespace acc::room_topology
