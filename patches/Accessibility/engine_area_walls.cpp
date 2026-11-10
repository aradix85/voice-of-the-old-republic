// Walkmesh extraction: reads each room's collision mesh from CSWSArea's room
// list, takes its faces to world space, and builds the area's wall cache
// (perimeter edges minus portals and duplicates, filtered in
// walkmesh_geometry.cpp) and its floor triangle cache.

#include "engine_area.h"

#include <windows.h>
#include <cstdint>

#include "engine_reads.h"
#include "engine_rebase.h"
#include "log.h"

namespace acc::engine {

namespace {

using PFN_CollisionMeshLocalToWorld = void(__thiscall*)(void* mesh, Vector* out, Vector* local);

// Under about 5 cm of floor-plan length an edge is a near-vertical step side
// with sub-centimetre drift: no wall in 2D, and it breaks the clustering.
constexpr float kMinEdgeXYLengthSq = 2.5e-3f;

// Every triangle side of every room, for the portal filter. K1's densest
// areas (Taris) have about 6000.
constexpr int kMaxTriSides = 16384;
TriEdge s_triSides[kMaxTriSides];

struct RoomMesh {
    void*          surfaceMesh;
    Vector*        vertices;
    uint32_t       faceCount;
    unsigned char* faces;      // kWalkmeshFaceStride apart, three uint32 vertex indices first
    int*           adjacency;  // int[3] per face, -1 = no neighbour; on the CSWRoomSurfaceMesh
    uint32_t*      materials;  // surfacemat.2da row per face
};

struct Face {
    Vector corner[3];  // world space
    int    adj[3];
    int    material;   // -1 when the mesh has none
};

bool ReadRoomMesh(void* surfaceMesh, RoomMesh* m) {
    if (!surfaceMesh) return false;
    auto* base = static_cast<unsigned char*>(surfaceMesh);
    __try {
        m->surfaceMesh = surfaceMesh;
        m->vertices  = *reinterpret_cast<Vector**>(base + kCollisionMeshVerticesOffset);
        m->faceCount = *reinterpret_cast<uint32_t*>(base + kCollisionMeshFaceCountOffset);
        m->faces     = *reinterpret_cast<unsigned char**>(base + kCollisionMeshFacesOffset);
        m->adjacency = *reinterpret_cast<int**>(base + kSurfaceMeshAdjacenciesOffset);
        m->materials = *reinterpret_cast<uint32_t**>(base + kCollisionMeshMaterialsOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return m->vertices && m->faces && m->faceCount > 0;
}

// CCollisionMesh::LocalToWorld. On a fault the local point stays, which is
// right for world_coords=1, the usual case for room walkmeshes.
void LocalToWorld(void* surfaceMesh, Vector* p) {
    Vector local = *p;
    __try {
        reinterpret_cast<PFN_CollisionMeshLocalToWorld>(kAddrCollisionMeshLocalToWorld)(
            surfaceMesh, p, &local);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *p = local;
    }
}

// False on a fault: the face is skipped.
bool ReadFace(const RoomMesh& m, uint32_t f, Face* out) {
    __try {
        auto* index =
            reinterpret_cast<uint32_t*>(m.faces + static_cast<size_t>(f) * kWalkmeshFaceStride);
        for (int k = 0; k < 3; ++k) {
            out->corner[k] = m.vertices[index[k]];
            out->adj[k] = m.adjacency ? m.adjacency[f * 3 + k] : 0;
        }
        out->material = m.materials ? static_cast<int>(m.materials[f]) : -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    for (int k = 0; k < 3; ++k) LocalToWorld(m.surfaceMesh, &out->corner[k]);
    return true;
}

// fn(mesh, roomIndex) for every room with a readable surface mesh.
template <typename Fn>
void ForEachRoomMesh(void* area, Fn fn) {
    void* rooms = nullptr;
    uint32_t count = 0;
    if (!TryReadPtr(area, kAreaRoomsOffset, &rooms) ||
        !TryReadU32(area, kAreaRoomCountOffset, &count) || !rooms) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        void* room = static_cast<unsigned char*>(rooms) + static_cast<size_t>(i) * kRoomStride;
        void* surfaceMesh = nullptr;
        RoomMesh m{};
        if (TryReadPtr(room, kRoomSurfaceMeshOffset, &surfaceMesh) &&
            ReadRoomMesh(surfaceMesh, &m)) {
            fn(m, static_cast<int>(i));
        }
    }
}

// fn(face, side, a, b) for each side of each face with a floor-plan
// footprint.
template <typename Fn>
void ForEachSide(const RoomMesh& m, Fn fn) {
    Face face;
    for (uint32_t f = 0; f < m.faceCount; ++f) {
        if (!ReadFace(m, f, &face)) continue;
        for (int e = 0; e < 3; ++e) {
            const Vector& a = face.corner[e];
            const Vector& b = face.corner[(e + 1) % 3];
            if (DistXYSq(a, b) >= kMinEdgeXYLengthSq) fn(face, e, a, b);
        }
    }
}

// Fills s_triSides; returns how many it holds.
int CollectTriSides(void* area) {
    int found = 0;
    ForEachRoomMesh(area, [&](const RoomMesh& m, int room) {
        ForEachSide(m, [&](const Face&, int, const Vector& a, const Vector& b) {
            if (found < kMaxTriSides) s_triSides[found] = TriEdge{a, b, room};
            ++found;
        });
    });
    if (found > kMaxTriSides) {
        acclog::Write("AreaWalls", "global edge index OVERFLOW: discovered=%d kMaxTriSides=%d "
                      "— portal filter may miss matches against the tail (raise the cap)",
                      found, kMaxTriSides);
        return kMaxTriSides;
    }
    return found;
}

}  // namespace

int BuildAreaWallCache(void* area, WallEdge* outBuf, int maxEdges) {
    if (!area) return 0;
    int total = 0;
    ForEachRoomMesh(area, [&](const RoomMesh& m, int room) {
        if (!m.adjacency) return;
        ForEachSide(m, [&](const Face& face, int e, const Vector& a, const Vector& b) {
            if (face.adj[e] != -1) return;
            if (outBuf && total < maxEdges) outBuf[total] = WallEdge{a, b, room, face.material};
            ++total;
        });
    });
    if (!outBuf || maxEdges <= 0) return total;  // count-only probe: the raw total

    const int written = total < maxEdges ? total : maxEdges;
    const int sideCount = CollectTriSides(area);
    const int kept = DropPortalEdges(outBuf, written, s_triSides, sideCount);
    acclog::Write("AreaWalls", "portal filter: emitted=%d -> kept=%d (dropped %d as portals via "
                  "coincidence against %d global edges)", written, kept, written - kept, sideCount);
    const int unique = DropSameRoomDuplicates(outBuf, kept);
    if (unique < kept) {
        acclog::Write("AreaWalls", "same-room dedup: dropped %d duplicate wall edges",
                      kept - unique);
    }
    return unique;
}

int BuildAreaFloorCache(void* area, FloorTri* outBuf, int maxTris) {
    if (!area) return 0;
    int total = 0;
    ForEachRoomMesh(area, [&](const RoomMesh& m, int) {
        Face face;
        for (uint32_t f = 0; f < m.faceCount; ++f) {
            if (!ReadFace(m, f, &face)) continue;
            if (outBuf && total < maxTris) {
                outBuf[total] = FloorTri{face.corner[0], face.corner[1], face.corner[2]};
            }
            ++total;
        }
    });
    return total;
}

}  // namespace acc::engine
