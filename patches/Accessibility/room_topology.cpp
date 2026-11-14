// The region graph of an area (see room_topology.h for its purpose). It is
// built from the engine's per-area nav graph (the same path_points /
// path_connections fields the engine itself uses for AI pathfinding); each
// node is classified by its CSR-adjacency degree:
//   - degree 1               → dead end pointing at its single neighbour
//   - degree 2, same room    → corridor (axis defined by both neighbours)
//   - degree 2, cross-room   → transition (doorway between two rooms)
//   - degree ≥ 3             → junction (comma-separated octant list)
//
// The nav graph is what BioWare ships and the engine trusts for path
// solving, the same ground truth the level designers worked with. Nodes are
// then merged into regions and each region labelled. This file holds the
// pipeline (BuildForArea), Reset and the public queries.

#include "room_topology_internal.h"

#include "log.h"
#include "spatial_wall_surfaces.h"  // GetCachedWalls — seam-filtered perimeter cache

namespace acc::room_topology {

namespace {

// Snap radius for runtime LookupAt: any player position more than this
// many world units (KOTOR = metres) from the nearest graph node falls
// back to the neutral "Area" label. 15m matches the typical room-diagonal in
// vanilla content while still catching the central plaza / exterior
// case where the nav graph is sparse. Tune from log evidence.
constexpr float kMaxSnapM = 15.0f;

// A closed door within kClosedDoorNearM of the position keeps the nodes
// behind it out of the primary pick: the walkmesh of the next room often
// starts a metre before its door, so a player standing at the closed door
// was placed in the region behind it (Black Vulkar Base, 0.8 m south of a
// door: "way blocked" to a footlocker 19 m on, with the door read as the
// way back). kDoorSideM: how far beside the line to a node the door's centre
// may lie and still stand in the way; half a door panel.
constexpr float kClosedDoorNearM = 4.0f;
constexpr float kDoorSideM       = 1.0f;

// Nav-graph build retry cap.
//
// transitions.cpp deliberately re-calls BuildForArea every tick until the
// graph builds, because the engine can have the area live before its
// walkmesh/path-point data is readable — a transient miss that resolves in
// a tick or two. That retry had no ceiling, so an area whose nav graph is
// *permanently* unreadable retried forever: measured at a steady 60/s for
// 82 unbroken seconds in module m40ad (2801 attempts, patch-20260729-103259
// .log, 10:33:46-10:35:08). m40ad is a cutscene-staging module with nothing
// walkable to read, so the miss is not transient and never resolves. Each
// attempt re-reads the engine's nav-graph metadata and logged a line, which
// is why the turret cutscene sequence stutters when ordinary area
// transitions do not.
//
// 120 ticks (~2s at 60fps) is a generous margin over the transient case the
// retry exists for, while turning the permanent case into a bounded cost.
// On giving up the area keeps an unbuilt graph, which is the same state it
// had while retrying — room_topology degrades to "open area" and no caller
// changes behaviour.
//
// The latch is per-area and is cleared on genuine area (re-)entry via
// ResetNavGraphRetry, NOT merely on an area-pointer change. That matters:
// the door-set stabiliser above carries a scar from precisely this shape of
// bug, where a bad result got locked in for the session and re-entering the
// area did not recover it.
constexpr int kNavGraphRetryCapTicks = 120;

struct NavGraphRetryState {
    void* area         = nullptr;
    int   failed_ticks = 0;
    bool  gave_up      = false;
};
NavGraphRetryState g_navgraph_retry;

}  // namespace

void Reset() {
    g_graph.area_owner = nullptr;
    g_graph.built      = false;
    g_graph.node_count = 0;
    g_graph.door_count = 0;
    g_graph.transition_count = 0;
    g_graph.cluster_links.clear();
    s_class_clear   = 0;
    s_class_door    = 0;
    s_class_blocked = 0;
    s_caveat1_hits  = 0;
    s_caveat2_hits  = 0;
    g_doors_stability = DoorStabilityState{};
}

bool HasGraphForArea(void* area) {
    return area != nullptr &&
           g_graph.built &&
           g_graph.area_owner == area;
}

void ResetNavGraphRetry() {
    g_navgraph_retry = NavGraphRetryState{};
}

namespace {

// Region adjacency for the exploration overview: which regions share a
// nav edge that does not cross a wall, and whether every such edge runs
// through a door. Labels never read this.
void BuildClusterLinks(const acc::engine::navgraph::NavGraphSnapshot& g, int n) {
    g_graph.cluster_links.clear();
    for (int i = 0; i < n; ++i) {
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        for (int e = lo; e < hi; ++e) {
            const int j = static_cast<int>(g.conns[e]);
            if (j == i || j < 0 || j >= n) continue;  // pairs dedupe below
            int ca = g_graph.node_cluster_id[i];
            int cb = g_graph.node_cluster_id[j];
            if (ca == cb) continue;
            if (ca > cb) std::swap(ca, cb);
            const EdgeResult er = ClassifyEdge(nullptr, g_graph.node_pos[i],
                                               g_graph.node_pos[j],
                                               "ClusterLinks");
            if (er.kind == kEdgeBlocked) continue;
            const bool viaDoor = er.kind == kEdgeDoor && er.doorIdx >= 0;
            ClusterLink* found = nullptr;
            for (ClusterLink& l : g_graph.cluster_links) {
                if (l.a == ca && l.b == cb) { found = &l; break; }
            }
            if (!found) {
                ClusterLink l;
                l.a       = ca;
                l.b       = cb;
                l.viaDoor = viaDoor;
                if (viaDoor) l.doorPos = g_graph.doors[er.doorIdx].pos;
                g_graph.cluster_links.push_back(l);
            } else if (found->viaDoor && !viaDoor) {
                found->viaDoor = false;  // a free way round the door
            }
        }
    }
}

}  // namespace

void BuildForArea(void* area) {
    if (!area) return;
    if (HasGraphForArea(area)) return;

    // Per-area retry accounting — see kNavGraphRetryCapTicks.
    if (g_navgraph_retry.area != area) {
        g_navgraph_retry.area         = area;
        g_navgraph_retry.failed_ticks = 0;
        g_navgraph_retry.gave_up      = false;
    }
    if (g_navgraph_retry.gave_up) return;

    Reset();
    g_graph.area_owner = area;

    acc::engine::navgraph::NavGraphSnapshot g;
    if (!acc::engine::navgraph::SnapshotNavGraph(area, g)) {
        // Log the first miss so an unreadable graph is still visible in the
        // log immediately, then stay quiet until we give up. Logging all 120
        // adds nothing — they are identical by construction.
        if (g_navgraph_retry.failed_ticks == 0) {
            acclog::Write("WallTopo",
                          "BuildForArea: nav graph empty / unreadable (areaPtr=%p) "
                          "— leaving graph unbuilt, retrying", area);
        }
        if (++g_navgraph_retry.failed_ticks >= kNavGraphRetryCapTicks) {
            g_navgraph_retry.gave_up = true;
            acclog::Write("WallTopo",
                          "BuildForArea: nav graph still unreadable after %d ticks "
                          "(areaPtr=%p) — giving up for this area; room topology "
                          "degrades to open-area until it is re-entered",
                          g_navgraph_retry.failed_ticks, area);
        }
        return;
    }
    g_navgraph_retry.failed_ticks = 0;

    int n = static_cast<int>(g.nodes.size());
    if (n > kMaxNodes) {
        acclog::Write("WallTopo",
                      "BuildForArea: nav graph has %d nodes — truncating to "
                      "kMaxNodes=%d; %d node%s dropped from classification "
                      "(label/exit data for them will be missing)",
                      static_cast<int>(g.nodes.size()), kMaxNodes,
                      static_cast<int>(g.nodes.size()) - kMaxNodes,
                      static_cast<int>(g.nodes.size()) - kMaxNodes == 1
                          ? "" : "s");
        n = kMaxNodes;
    }

    g_graph.node_count = n;
    for (int i = 0; i < n; ++i) {
        g_graph.node_pos[i]        = g.nodes[i].pos;
        g_graph.node_label[i].clear();
        g_graph.node_exits[i].clear();
        g_graph.node_sig[i]        = 0;
        g_graph.node_kind[i]       = kKindOpenArea;
        g_graph.node_cluster_id[i] = kClusterIdNone;
        g_graph.node_extent[i]     = 0.0f;
        g_graph.node_centre[i]     = g.nodes[i].pos;
        g_graph.node_footprint[i]  = 0.0f;
        g_graph.node_floor_z[i]    = 0.0f;
    }

    // Snapshot doors before classifying clusters — ClassifyCluster's
    // FindDoorOnEdge calls consume g_graph.doors[]. The initial snapshot
    // can race a partially-populated server-object array; the per-tick
    // MaybeRefreshDoors loop catches late-arriving doors. Initial
    // cluster classification is locked to whatever was visible now
    // (accepted lossy behaviour — late doors still show up as "Door"
    // exits via the per-edge query).
    SnapshotDoors(area);
    LogDoorSnapshotDetails(area);
    // Attach landmark map-notes to doors before any cluster labels get
    // rendered — RenderDoorDirection consults DoorRecord.landmarkName and
    // labels are baked once during the classification passes below. Also
    // flags matched landmarks in the transitions cache so the
    // proximity-fire path won't double-announce.
    AttachLandmarksToDoors(area);
    // Walk-through area-transition triggers — the door analog for outdoor
    // maps. Needs node_pos[] (filled above) for the nearest-node binding.
    SnapshotTransitionTriggers(area);
    g_doors_stability.last_count  = g_graph.door_count;
    g_doors_stability.streak      = 0;
    g_doors_stability.retry_ticks = 0;
    g_doors_stability.committed   = false;

    // Per-node shape features first, so every merge pass below can branch
    // on the same openness / room flags. They no longer drive separate
    // merge classes — they only decide, per node, whether it is "space"
    // (a place the player stands in) or "passage" (a link between places);
    // see the Pass 1 space/passage rule.
    BuildPasses bp;
    ComputeNodeShapeFeatures(g, n, bp.nodeOpen, bp.nodeRoom, bp.nodeFloorZ,
                             bp.nodeDeadEnd, bp.nodeAxisExt);
    // Persist the calibrated floor heights immediately — Pass 3's ramp-
    // exit annotation and (after build) LookupAt's zTrusted filters read
    // g_graph.node_floor_z directly.
    for (int i = 0; i < n; ++i) {
        g_graph.node_floor_z[i] = bp.nodeFloorZ[i];
    }

    // Every node starts in its own union-find class; the passes below merge.
    for (int i = 0; i < n; ++i) s_uf_parent[i] = i;

    ComputeChainStraightness(g, n, bp);

    // Wall cache for the edge clearness test (shared by Pass 1 and Pass 2).
    bp.haveGW = acc::spatial::wall_surfaces::GetCachedWalls(
                    bp.gw, bp.gwCount) && bp.gw && bp.gwCount > 0;

    PassCoreMerge(g, n, bp);
    PassCornerFold(g, n, bp);
    AbsorbAdjacentStragglers(g, n, bp);
    AbsorbStragglersByBoundingBox(area, g, n, bp);
    AbsorbOpenEndedPendants(g, n, bp);
    PassClassifyClusters(area, g, n, bp);

    // Freeze the UFFind roots into node_cluster_id[] now that all merge
    // passes have completed. After this point UFFind itself is no
    // longer consulted at runtime — LookupAt reads node_cluster_id
    // directly, so subsequent path-halving on UFFind can't shift the
    // observed id under a caller comparing against a stored value.
    for (int i = 0; i < n; ++i) {
        g_graph.node_cluster_id[i] = UFFind(i);
    }

    BuildClusterLinks(g, n);

    g_graph.built = true;
    acclog::Write("WallTopo",
                  "BuildForArea: area=%p nodes=%d clusters=%d "
                  "core-merge(space=%d corridor=%d) "
                  "absorb(adj=%d bbox=%d) multi-node-clusters=%d "
                  "(dead=%d corridor=%d junction=%d open=%d)",
                  area, n, bp.clusters,
                  bp.coreSpace, bp.coreCorridor,
                  bp.absorbAdj, bp.bboxAbsorbed, bp.multiNodeClusters,
                  bp.deadEnds, bp.corridors, bp.junctions, bp.openAreas);

    // Edge-classification summary. Counts are multi-fire (each graph
    // edge is classified by every pass that examines it — Pass 1
    // core-merge + Pass 2 absorb vetoes, then Pass 3 external-edge
    // collection), so
    // reading them as raw per-edge truth would overcount. They're a
    // ratio/anomaly signal: blocked > 0 confirms the wall primitive
    // is catching at least some wall-crossing edges; caveat-1 hits > 0
    // signals K1 violates the walkmesh-holes-for-doors convention here
    // (rare, high-signal); caveat-2 candidates is a per-area noise
    // floor for "Clear edges between distinct .wok rooms".
    acclog::Write("WallTopo",
                  "  edge-classification summary (multi-counted across "
                  "passes): clear=%d door=%d blocked=%d | "
                  "caveat-1 (door-in-wall)=%d caveat-2 (cross-room "
                  "candidates)=%d",
                  s_class_clear, s_class_door, s_class_blocked,
                  s_caveat1_hits, s_caveat2_hits);

    // ===== End-of-build diagnostics (observation only, no behaviour) =====
    // Per-node topology signatures (degree / triangles / 4-cycles / reach)
    // for future gate tuning.
    LogTopologyMetrics(g, n);
    // Phantom-wall confirmation: cross-check the engine's own nav graph
    // against the cached walls (3D-aware). Any nav edge that crosses a
    // cached wall proves that wall is phantom — see LogNavWallCrossings.
    LogNavWallCrossings(g);
    // Per multi-node cluster, dump member adjacency + ClassifyEdge verdicts.
    LogClusterMemberAdjacency(g, n);
    // Final per-node cluster/kind/sig/label table.
    DumpGraphToLog();
}

void DumpGraphToLog() {
    if (!g_graph.built) {
        acclog::Write("WallTopo", "DumpGraphToLog: no graph built");
        return;
    }
    for (int i = 0; i < g_graph.node_count; ++i) {
        int root = UFFind(i);
        acclog::Write("WallTopo",
                      "  node[%d] cluster=%d kind=%d sig=%d "
                      "pos=(%.1f,%.1f,%.1f) label=\"%s\"",
                      i, root, g_graph.node_kind[i], g_graph.node_sig[i],
                      g_graph.node_pos[i].x, g_graph.node_pos[i].y,
                      g_graph.node_pos[i].z, g_graph.node_label[i].c_str());
    }
}

bool GetClusterInfo(void* area, int clusterId, ClusterInfo& out) {
    out = ClusterInfo{};
    if (clusterId < 0) return false;
    if (!HasGraphForArea(area)) return false;
    for (int i = 0; i < g_graph.node_count; ++i) {
        if (g_graph.node_cluster_id[i] != clusterId) continue;
        out.kind       = g_graph.node_kind[i];
        out.extentM    = g_graph.node_extent[i];
        out.footprintM = g_graph.node_footprint[i];
        out.centre     = g_graph.node_centre[i];
        out.largeArea  = out.extentM >= kLargeAreaExtentMeters;
        return true;
    }
    return false;
}

bool NearestClusterNode(void* area, int clusterId, const Vector& p, Vector& out) {
    if (clusterId < 0 || !HasGraphForArea(area)) return false;
    int best = -1;
    for (int i = 0; i < g_graph.node_count; ++i) {
        if (g_graph.node_cluster_id[i] != clusterId) continue;
        if (best < 0 || DistXYSq(g_graph.node_pos[i], p) < DistXYSq(g_graph.node_pos[best], p)) {
            best = i;
        }
    }
    if (best < 0) return false;
    out = g_graph.node_pos[best];
    return true;
}

bool ListClusters(void* area, std::vector<ClusterSummary>& out) {
    out.clear();
    if (!HasGraphForArea(area)) return false;
    for (int i = 0; i < g_graph.node_count; ++i) {
        if (g_graph.node_filtered[i]) continue;
        const int id = g_graph.node_cluster_id[i];
        bool seen = false;
        for (const ClusterSummary& c : out) {
            if (c.id == id) { seen = true; break; }
        }
        if (seen) continue;
        ClusterSummary c;
        c.id     = id;
        c.kind   = g_graph.node_kind[i];
        c.centre = g_graph.node_centre[i];
        out.push_back(c);
    }
    return true;
}

bool GetClusterLinks(void* area, std::vector<ClusterLink>& out) {
    out.clear();
    if (!HasGraphForArea(area)) return false;
    out = g_graph.cluster_links;
    return true;
}

namespace {

// The labelled nodes nearest to a position, by floor-plan distance: the
// engine stores every path-graph node at z = 0, so a 3D distance would pin
// elevated areas (the Taris sewers platform at z 28) to the neutral "Area".
struct NodeScan {
    int   best = -1;          // reachable, not filtered: the primary pick
    float bestSq = 1e30f;
    int   pastDoor = -1;      // the same, but past the closed door the caller
    float pastDoorSq = 1e30f; //   stands at: taken only without a primary pick
    int   filtered = -1;      // reachable, dropped by the walkmesh-shape
    float filteredSq = 1e30f; //   gate: the rescue pick
    int   blocked = -1;       // nearest unreachable one, for the log
    float blockedSq = 1e30f;
    bool  blockedByZ = false;
    int   blockedSeen = 0;
};

// A node is reachable when it is on the caller's floor (zTrusted: its
// calibrated floor within the wall test's own z tolerance, so "same floor"
// means one thing in both filters) and no wall lies between
// (requireWallReachable: against the seam-filtered wall cache, so a room
// does not speak the label of the corridor next door). With a trusted z
// the ray runs floor to floor and other-storey walls drop out; without it
// the test is plain 2D, for cursor callers. closedDoor: the closed door near
// the caller, or null.
NodeScan ScanNodes(const Vector& worldPos, bool requireWallReachable, bool zTrusted,
                   const Vector* closedDoor) {
    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    const bool testWalls =
        acc::spatial::wall_surfaces::GetCachedWalls(walls, wallCount) &&
        requireWallReachable;

    NodeScan s;
    for (int i = 0; i < g_graph.node_count; ++i) {
        if (g_graph.node_label[i].empty()) continue;
        const float d2 = DistXYSq(g_graph.node_pos[i], worldPos);

        const bool zVetoed =
            zTrusted && std::fabs(worldPos.z - g_graph.node_floor_z[i]) >
                            acc::engine::kWallCrossZToleranceM;
        bool reachable = !zVetoed;
        if (reachable && testWalls) {
            Vector hit{};
            Vector nodeEnd = g_graph.node_pos[i];
            if (zTrusted) nodeEnd.z = g_graph.node_floor_z[i];
            reachable = !acc::engine::SegmentCrossesWalkmesh(
                walls, wallCount, worldPos, nodeEnd, hit);
        }

        if (g_graph.node_filtered[i]) {
            if (reachable && d2 < s.filteredSq) {
                s.filteredSq = d2;
                s.filtered = i;
            }
        } else if (reachable) {
            if (closedDoor &&
                StandsBetweenXY(*closedDoor, worldPos, g_graph.node_pos[i], kDoorSideM)) {
                if (d2 < s.pastDoorSq) {
                    s.pastDoorSq = d2;
                    s.pastDoor = i;
                }
            } else if (d2 < s.bestSq) {
                s.bestSq = d2;
                s.best = i;
            }
        } else {
            ++s.blockedSeen;
            if (d2 < s.blockedSq) {
                s.blockedSq = d2;
                s.blocked = i;
                s.blockedByZ = zVetoed;
            }
        }
    }
    return s;
}

// Logged only when the filter rejected a candidate nearer than the pick, or
// rejected every one (a sign the wall cache or the position is wrong).
// Callers ask for it on room changes, not per tick.
void LogScan(const NodeScan& s, const Vector& worldPos) {
    if (s.pastDoor >= 0 && s.best >= 0 && s.pastDoorSq < s.bestSq) {
        acclog::Write(
            "WallTopo",
            "LookupAt DOOR-FILTERED at (%.1f,%.1f,%.1f): nearest candidate node[%d] "
            "(%.1f,%.1f) \"%s\" at %.1fm is past a closed door; picked node[%d] "
            "(%.1f,%.1f) \"%s\" at %.1fm instead",
            worldPos.x, worldPos.y, worldPos.z, s.pastDoor,
            g_graph.node_pos[s.pastDoor].x, g_graph.node_pos[s.pastDoor].y,
            g_graph.node_label[s.pastDoor].c_str(), std::sqrt(s.pastDoorSq), s.best,
            g_graph.node_pos[s.best].x, g_graph.node_pos[s.best].y,
            g_graph.node_label[s.best].c_str(), std::sqrt(s.bestSq));
    }
    if (s.blocked >= 0 && (s.best < 0 || s.blockedSq < s.bestSq)) {
        acclog::Write(
            "WallTopo",
            "LookupAt WALL-FILTERED at (%.1f,%.1f,%.1f): nearest candidate node[%d] "
            "(%.1f,%.1f) \"%s\" at %.1fm is %s; picked node[%d] (%.1f,%.1f) \"%s\" "
            "at %.1fm instead (blocked-seen=%d)",
            worldPos.x, worldPos.y, worldPos.z, s.blocked,
            g_graph.node_pos[s.blocked].x, g_graph.node_pos[s.blocked].y,
            g_graph.node_label[s.blocked].c_str(), std::sqrt(s.blockedSq),
            s.blockedByZ ? "on another level (z-veto)" : "behind a wall", s.best,
            s.best >= 0 ? g_graph.node_pos[s.best].x : 0.0f,
            s.best >= 0 ? g_graph.node_pos[s.best].y : 0.0f,
            s.best >= 0 ? g_graph.node_label[s.best].c_str() : "(none)",
            s.best >= 0 ? std::sqrt(s.bestSq) : -1.0f, s.blockedSeen);
    }
    if (s.best < 0 && s.blockedSeen > 0) {
        acclog::Write(
            "WallTopo",
            "LookupAt ALL-BLOCKED at (%.1f,%.1f,%.1f): every labelled node (%d "
            "candidates) was wall-filtered. Falling back to neutral Area. Possible "
            "overfire - check wall cache vs player position.",
            worldPos.x, worldPos.y, worldPos.z, s.blockedSeen);
    }
}

bool InSnapRange(int node, float distSq) {
    return node >= 0 && distSq <= kMaxSnapM * kMaxSnapM;
}

// The closed door within kClosedDoorNearM of `p`, if any. The snapshot's
// positions answer "is there a door here at all" without touching the
// engine; only then is the live door asked whether it is closed.
bool ClosedDoorNear(void* area, const Vector& p, Vector& outDoor) {
    bool any = false;
    for (int i = 0; i < g_graph.door_count && !any; ++i) {
        any = DistXYSq(g_graph.doors[i].pos, p) <= kClosedDoorNearM * kClosedDoorNearM;
    }
    if (!any) return false;
    return acc::engine::NearestDoor(area, p, kClosedDoorNearM, [](void* door) {
        return !acc::engine::IsDoorOpen(door) && !acc::engine::IsDoorCosmetic(door);
    }, &outDoor) != nullptr;
}

}  // namespace

bool LookupAt(void* area, const Vector& worldPos,
              std::string& outLabel, int& outSig,
              int& outClusterId,
              bool allowDiagLog,
              bool requireWallReachable,
              bool zTrusted,
              std::vector<ExitMark>* outExits) {
    outLabel.clear();
    if (outExits) outExits->clear();
    outSig = 0;
    outClusterId = kClusterIdNone;
    if (!HasGraphForArea(area)) return false;
    if (g_graph.node_count <= 0) return false;

    // Walking callers only: the map cursor is not stopped by a door.
    Vector doorPos{};
    const bool atClosedDoor =
        requireWallReachable && ClosedDoorNear(area, worldPos, doorPos);

    const NodeScan s = ScanNodes(worldPos, requireWallReachable, zTrusted,
                                 atClosedDoor ? &doorPos : nullptr);
    if (allowDiagLog) LogScan(s, worldPos);

    // The primary pick wins when in range, then the one past the closed
    // door. Otherwise the rescue pick: an alcove the shape gate misjudged,
    // with no labelled cluster in range. The merge passes upstream absorb
    // wall-curve artefacts inside inhabited regions, so a surviving filtered
    // node is almost always a real alcove.
    int pick = -1;
    if (InSnapRange(s.best, s.bestSq)) {
        pick = s.best;
    } else if (InSnapRange(s.pastDoor, s.pastDoorSq)) {
        pick = s.pastDoor;
    } else if (InSnapRange(s.filtered, s.filteredSq)) {
        pick = s.filtered;
        if (allowDiagLog) {
            acclog::Write(
                "WallTopo",
                "LookupAt RESCUE at (%.1f,%.1f,%.1f): no unfiltered candidate in "
                "range (best primary %.1fm); using filtered node[%d] (%.1f,%.1f) "
                "\"%s\" at %.1fm",
                worldPos.x, worldPos.y, worldPos.z,
                s.best >= 0 ? std::sqrt(s.bestSq) : -1.0f, pick,
                g_graph.node_pos[pick].x, g_graph.node_pos[pick].y,
                g_graph.node_label[pick].c_str(), std::sqrt(s.filteredSq));
        }
    }
    if (pick >= 0) {
        outLabel     = g_graph.node_label[pick];
        outSig       = g_graph.node_sig[pick];
        outClusterId = g_graph.node_cluster_id[pick];
        if (outExits) *outExits = g_graph.node_exits[pick];
        return true;
    }

    // Candidates, but none in range: the neutral "Area", so the player gets
    // one stable cue. False only when the graph had no candidate at all.
    if (s.best < 0 && s.pastDoor < 0 && s.filtered < 0 && s.blockedSeen == 0) return false;
    outLabel = acc::strings::Get(acc::strings::Id::AreaNoun);
    outSig = kKindOpenArea & 0xff;
    outClusterId = kClusterIdOpenArea;
    return true;
}

}  // namespace acc::room_topology
