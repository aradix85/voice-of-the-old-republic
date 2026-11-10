// Rakatan temple floor-plate puzzle assist — see floor_puzzle.h for the
// puzzle model (decompiled k_punk_floor01..09 / k_punk_reset scripts).

#include "floor_puzzle.h"

#include <windows.h>
#include <cmath>
#include <cstring>
#include <string>

#include "engine_area.h"
#include "engine_keymap.h"   // GameActionVk: the game's live solo-mode bind
#include "engine_player.h"
#include "floor_puzzle_plates.h"
#include "hotkeys.h"
#include "log.h"
#include "speech.h"
#include "strfmt.h"
#include "strings.h"
#include "view_mode.h"

namespace acc::floor_puzzle {

namespace {

using acc::strings::Get;
using acc::strings::Id;

// Poll cadence. Plate polygons are ~1.6-2.0m and walk speed ~5m/s, so
// 150ms bounds the detection lag to well under one plate length; the
// toggle scripts run inside the engine's own trigger dispatch, so the
// board read one poll later already sees the new state.
constexpr DWORD kScanIntervalMs = 150;

// The only module this feature exists in.
constexpr char kPuzzleModule[] = "unk_m44ab";

// NWScript local-boolean index the plate scripts store the lit state in
// (GetLocalBoolean(panel, 10) throughout k_punk_floor05's decompile).
constexpr int kLitBooleanIndex = 10;

// swkotor.ini [Keymapping] id for Solo Mode: keymap.2da row 7
// ("PartyActive", default V) + the 200 offset verified via row 64
// STEALTH <-> Action264.
constexpr int kSoloModeActionId = 207;
constexpr int kSoloModeDefaultVk = 'V';

// Nearest-plate announcements: re-announce when the line changes, but not
// faster than this (walking a lane past the grid changes it every ~2m;
// unthrottled that is chatter, throttled it reads as a position stream).
constexpr DWORD kNearestMinGapMs = 700;

// One-shot intro fires when the leader first comes this close to the
// grid centre (the room is ~15m long; 12m covers "walked in").
constexpr float kIntroRadiusM = 12.0f;
constexpr int   kCenterPlate  = 4;   // FloorPanel05

// Board-delta announcements within this window of the leader entering a
// plate are attributed to that step; outside it they were caused by a
// follower walking the grid.
constexpr DWORD kLeaderToggleWindowMs = 600;

// Entry announcements are HELD up to this long to merge with the toggle
// delta into ONE utterance: the plate script usually runs a poll after we
// see the position change, and speaking the entry at once let the delta
// line interrupt it mid-word one tick later.
constexpr DWORD kEntryMergeWindowMs = 500;

// Grid-position word per plate index (tag number - 1). Rows run
// north->south, columns west->east: kFloorPanel01 is the north-west
// corner (GIT geometry, m44ab), 05 the centre, 09 the south-east corner.
constexpr Id kPlateWord[kPlateCount] = {
    Id::DirNorthwest, Id::DirNorth,  Id::DirNortheast,
    Id::DirWest,      Id::PlateCenterWord, Id::DirEast,
    Id::DirSouthwest, Id::DirSouth,  Id::DirSoutheast,
};

// Per-visit state, reset whenever the area changes.
struct Visit {
    Plate plates[kTrackCount];
    bool  cacheReady = false;
    bool  lit[kPlateCount] = {};
    bool  haveLit = false;
    int   curPlate = -1;            // -1 none, 0..8 plate, 9 reset
    std::string lastNearestLine;    // last spoken off-plate announcement
    DWORD nearestSpokenAt = 0;
    DWORD lastEntryAt = 0;          // leader stepped onto a plate at this tick
    int   pendingEntry = -1;        // held entry announcement; -1 none
    DWORD pendingEntryAt = 0;
    bool  introSpoken = false;
    bool  solvedQuiet = false;
};

Visit g_visit;
void* g_area         = nullptr;
bool  g_isPuzzleArea = false;
DWORD g_lastScan     = 0;

// ---- Naming and phrases -----------------------------------------------------

// Short name: the bare direction word ("North-West", "Center") used inside
// delta lists; kResetIndex gets the full reset-plate noun.
const char* ShortName(int idx) {
    if (idx == kResetIndex) return Get(Id::PlateResetName);
    if (idx < 0 || idx >= kPlateCount) return "";
    return Get(kPlateWord[idx]);
}

// Full name: "Plate <word>" for grid plates, the reset noun as-is.
std::string FullName(int idx) {
    if (idx == kResetIndex) return std::string(Get(Id::PlateResetName));
    return acc::strfmt::Format(Get(Id::FmtPlateName), ShortName(idx));
}

// World-cardinal steps to the plate's AREA, e.g. "2 metres north": the
// puzzle is walked with cardinal stutter-steps (align north once, then
// forward/back + strafe) toward the plate as a whole, not its centre. An
// axis is spoken only when the player is outside the plate's span on it,
// so standing in the gap beside a plate gives one word ("1 metre north" =
// one step north lands on it). Empty on the plate.
std::string AxisPhrase(const Vector& playerPos, const Plate& p) {
    const SpanOffset off = OffsetToSpan(p, playerPos);
    std::string out;
    auto add = [&out](float d, Id plus, Id minus) {
        if (d == 0.0f) return;
        int m = static_cast<int>(std::fabs(d) + 0.5f);
        if (!out.empty()) out += ", ";
        out += acc::strfmt::Format(Get(Id::FmtRouteSegment), m < 1 ? 1 : m,
                                   Get(d > 0 ? plus : minus));
    };
    add(off.north, Id::DirNorth, Id::DirSouth);
    add(off.east, Id::DirEast, Id::DirWest);
    return out;
}

std::string WithLitCount(std::string list, int litCount) {
    if (!list.empty()) list += ". ";
    return list + acc::strfmt::Format(Get(Id::FmtPlateLitCount), litCount);
}

// Itemized flip list, phrased as the TRANSITION each plate just made
// ("North-West lights up, West goes dark") + running lit count.
std::string DeltaText(const bool* flipped, const bool* lit, int litCount) {
    if (litCount == 0) return Get(Id::PlatesAllDark);
    std::string out;
    for (int i = 0; i < kPlateCount; ++i) {
        if (!flipped[i]) continue;
        if (!out.empty()) out += ", ";
        out += acc::strfmt::Format(
            Get(lit[i] ? Id::FmtPlateLit : Id::FmtPlateDark), ShortName(i));
    }
    return WithLitCount(out, litCount);
}

// Live board summary spoken on demand (R key): every lit plate plus the
// count, so the player can re-orient at any point ("North-West, Center,
// South-East. 3 of 9 lit."); nothing lit -> "all dark".
std::string BoardStateText(const bool* lit, int litCount) {
    if (litCount == 0) return Get(Id::PlatesAllDark);
    std::string out;
    for (int i = 0; i < kPlateCount; ++i) {
        if (!lit[i]) continue;
        if (!out.empty()) out += ", ";
        out += ShortName(i);
    }
    return WithLitCount(out, litCount);
}

// ---- Cache -------------------------------------------------------------------

// The GIT authors trigger polygons as position + local offsets and the
// engine translates them to world on load. Defend against a local-space
// read: room coordinates here are all > 60, local offsets all within a
// few metres of 0.
void ToWorldIfLocal(void* obj, const char* tag, Plate& p) {
    if (std::fabs(p.poly[0].x) >= 30.0f || std::fabs(p.poly[0].y) >= 30.0f) {
        return;
    }
    Vector base{};
    if (!acc::engine::GetObjectPosition(obj, base)) return;
    for (int i = 0; i < p.vertCount; ++i) {
        p.poly[i].x += base.x;
        p.poly[i].y += base.y;
        p.poly[i].z += base.z;
    }
    acclog::Write("FloorPuzzle",
                  "trigger %s geometry was local-space; translated by (%.1f,%.1f)",
                  tag, base.x, base.y);
}

// True when the trigger is a puzzle plate with usable geometry.
bool AddTriggerPlate(void* obj, const char* tag) {
    int slot = TrigSlotForTag(tag);
    if (slot < 0) return false;
    Plate& p = g_visit.plates[slot];
    p.vertCount = acc::engine::GetTriggerGeometry(obj, p.poly, kMaxPolyVerts);
    if (p.vertCount < 3) return false;
    ToWorldIfLocal(obj, tag, p);
    p.trigHandle = acc::engine::GetObjectHandle(obj);
    ComputeCenterAndSpan(p);
    return true;
}

// True when the placeable is a plate's lit-state carrier.
bool AddPlatePlaceable(void* obj, const char* tag) {
    int slot = PlcSlotForTag(tag);
    if (slot < 0) return false;
    g_visit.plates[slot].plcHandle = acc::engine::GetObjectHandle(obj);
    return true;
}

bool BuildCache(void* area) {
    using acc::engine::GameObjectKind;
    int trigFound = 0, plcFound = 0;
    acc::engine::AreaObjectIterator iter(area);
    void* obj = nullptr;
    while ((obj = iter.Next()) != nullptr) {
        int kind = acc::engine::GetObjectKind(obj);
        bool trigger = kind == static_cast<int>(GameObjectKind::Trigger);
        if (!trigger && kind != static_cast<int>(GameObjectKind::Placeable)) {
            continue;
        }
        char tag[64];
        if (!acc::engine::GetObjectTag(obj, tag, sizeof(tag))) continue;
        if (trigger) {
            trigFound += AddTriggerPlate(obj, tag) ? 1 : 0;
        } else {
            plcFound += AddPlatePlaceable(obj, tag) ? 1 : 0;
        }
    }
    if (trigFound < kTrackCount || plcFound < kPlateCount) {
        acclog::Write("FloorPuzzle",
                      "cache incomplete: triggers=%d/%d placeables=%d/%d — "
                      "retrying next scan",
                      trigFound, kTrackCount, plcFound, kPlateCount);
        return false;
    }
    // Geometry sanity line: the NW plate sits near (71.1, 92.3) world.
    const Plate* plates = g_visit.plates;
    acclog::Write("FloorPuzzle",
                  "cache ready: plate01 verts=%d center=(%.1f,%.1f) "
                  "reset center=(%.1f,%.1f)",
                  plates[0].vertCount, plates[0].center.x, plates[0].center.y,
                  plates[kResetIndex].center.x, plates[kResetIndex].center.y);
    return true;
}

// ---- Live reads --------------------------------------------------------------

// The live lit/dark state off the plate placeables. Returns the lit count;
// fills lit[0..kPlateCount). Cheap enough for the per-frame R path.
int ReadLiveBoard(bool* lit) {
    int litCount = 0;
    for (int i = 0; i < kPlateCount; ++i) {
        void* plc = acc::engine::ResolveServerObjectHandle(
            g_visit.plates[i].plcHandle);
        lit[i] = plc &&
                 acc::engine::GetObjectLocalBoolean(plc, kLitBooleanIndex);
        if (lit[i]) ++litCount;
    }
    return litCount;
}

// Board-navigation position: the view-mode virtual cursor when view mode
// is active, otherwise the party leader. In view mode the leader is
// frozen, so the cursor lets "Stepped on Plate X" and the nearest-plate
// offsets track where the player is scanning. The cursor never steps on a
// plate, so no lit/dark toggle (and thus no delta) can fire off it.
bool GetNavPosition(Vector& out) {
    if (acc::view_mode::IsActive() &&
        acc::view_mode::TryGetCursorPosition(out)) {
        return true;
    }
    return acc::engine::GetPlayerPosition(out);
}

// ---- Tick steps ------------------------------------------------------------

// On-demand board read (R), every frame, ABOVE the scan throttle: the
// rising edge from Pressed() lives for one OnUpdate tick and the throttle
// skips most frames. In the puzzle room bare R reports the board (no
// dialog panel here); input_pipeline swallows the engine's native R so it
// can't ALSO interact.
void MaybeReadBoard() {
    if (!IsActive() ||
        !acc::hotkeys::Pressed(acc::hotkeys::Action::DialogRepeatLine)) {
        return;
    }
    bool lit[kPlateCount];
    int  litCount = ReadLiveBoard(lit);
    std::string board = BoardStateText(lit, litCount);
    acc::speech::Speak(board.c_str(), /*interrupt=*/true);
    acclog::Write("FloorPuzzle", "R board read -> [%s] (lit=%d)",
                  board.c_str(), litCount);
}

// Follows the current area and builds the plate cache inside the puzzle
// module. True when the assist runs this scan.
bool TrackArea() {
    void* area = acc::engine::GetCurrentArea();
    if (area != g_area) {
        g_area = area;
        g_visit = Visit{};
        char resname[64];
        g_isPuzzleArea =
            area &&
            acc::engine::GetCurrentAreaResName(resname, sizeof(resname)) &&
            _stricmp(resname, kPuzzleModule) == 0;
        if (g_isPuzzleArea) {
            acclog::Write("FloorPuzzle", "puzzle module entered (%s)", resname);
        }
    }
    if (!g_isPuzzleArea) return false;
    if (!g_visit.cacheReady) g_visit.cacheReady = BuildCache(area);
    return g_visit.cacheReady;
}

// One scan of the board: the live lit state and the plates that flipped
// since the previous scan.
struct BoardScan {
    bool lit[kPlateCount];
    bool flipped[kPlateCount];
    int  litCount;
    bool anyFlip;
};

BoardScan ReadBoardScan() {
    BoardScan b{};
    b.litCount = ReadLiveBoard(b.lit);
    if (g_visit.haveLit) {
        for (int i = 0; i < kPlateCount; ++i) {
            b.flipped[i] = (b.lit[i] != g_visit.lit[i]);
            b.anyFlip |= b.flipped[i];
        }
    } else {
        // First read for this area visit. An already all-lit board means
        // the puzzle was solved on an earlier visit — stay quiet without
        // announcing success again.
        if (b.litCount == kPlateCount) {
            g_visit.solvedQuiet = true;
            acclog::Write("FloorPuzzle",
                          "board already all-lit on entry — quiet mode");
        }
        g_visit.haveLit = true;
    }
    std::memcpy(g_visit.lit, b.lit, sizeof(g_visit.lit));
    return b;
}

// Speaks the solve once; true while the module stays quiet after it.
bool QuietAfterSolve(const BoardScan& b) {
    if (!g_visit.solvedQuiet && b.litCount == kPlateCount && b.anyFlip) {
        g_visit.solvedQuiet = true;
        acc::speech::Speak(Get(Id::FloorPuzzleSolved), /*interrupt=*/true);
        acclog::Write("FloorPuzzle", "solved — module going quiet");
    }
    return g_visit.solvedQuiet;
}

// Where the player is this scan and the plate they stand on.
struct NavScan {
    Vector pos;
    bool   havePos;
    int    plate;     // -1 none, 0..8 plate, 9 reset
    bool   entered;   // stepped onto a plate this scan
};

NavScan TrackNavPlate(DWORD now) {
    NavScan n{};
    n.plate = -1;
    n.havePos = GetNavPosition(n.pos);
    if (!n.havePos) return n;
    for (int i = 0; i < kTrackCount; ++i) {
        if (PointInPoly(g_visit.plates[i], n.pos.x, n.pos.y)) {
            n.plate = i;
            break;
        }
    }
    n.entered = n.plate != g_visit.curPlate && n.plate >= 0;
    // Reset counts too: its script darkens the board, and that delta
    // must not read as a follower toggle when it lands a poll later.
    if (n.entered) g_visit.lastEntryAt = now;
    // Stepping off into open floor: drop the nearest anchor so the
    // off-plate stream re-announces the closest plate once.
    if (n.plate < 0 && g_visit.curPlate >= 0) g_visit.lastNearestLine.clear();
    g_visit.curPlate = n.plate;
    return n;
}

// One-shot orientation line near the grid, with the board-read key, the
// in-story clue and, when followers would toggle plates too, the solo-mode
// key. True when spoken.
bool MaybeSpeakIntro(const Vector& playerPos) {
    const Vector& centre = g_visit.plates[kCenterPlate].center;
    if (DistXYSq(playerPos, centre) > kIntroRadiusM * kIntroRadiusM) {
        return false;
    }
    g_visit.introSpoken = true;
    std::string line(Get(Id::FloorPuzzleIntro));
    line += " ";
    line += acc::strfmt::Format(
        Get(Id::FmtFloorReadHint),
        acc::hotkeys::Describe(acc::hotkeys::Action::DialogRepeatLine));
    line += " ";
    line += Get(Id::FloorPuzzleStoryHint);
    uint32_t followers[12];
    int followerCount = acc::engine::GetPartyMembers(
        followers, static_cast<int>(sizeof(followers) / sizeof(followers[0])));
    if (followerCount > 0 && !acc::engine::GetSoloMode()) {
        int vk = acc::engine_keymap::GameActionVk(kSoloModeActionId);
        if (vk == 0) vk = kSoloModeDefaultVk;
        line += " ";
        line += acc::strfmt::Format(Get(Id::FmtFloorSoloHint),
                                    acc::hotkeys::VkLabel(vk));
    }
    acc::speech::Speak(line.c_str(), /*interrupt=*/true);
    acclog::Write("FloorPuzzle", "intro spoken (followers=%d solo=%d)",
                  followerCount, acc::engine::GetSoloMode() ? 1 : 0);
    return true;
}

// The reset plate gets no explanatory sentence: its name plus the
// "all plates dark" delta that follows says everything.
std::string EntryText(int plate) {
    return acc::strfmt::Format(Get(Id::FmtPlateEntered),
                               FullName(plate).c_str());
}

// Entry and toggle speech. True when a line was spoken.
bool AnnounceEntryOrDelta(const BoardScan& b, const NavScan& n, DWORD now) {
    // A flip within the attribution window of the leader's own step is
    // "their" toggle; a flip outside it means a follower walked the grid.
    bool leaderCaused = g_visit.lastEntryAt != 0 &&
                        (now - g_visit.lastEntryAt) <= kLeaderToggleWindowMs;

    if (n.entered) {
        if (g_visit.pendingEntry >= 0) {
            // The previous entry never saw its delta (fast crossing) —
            // speak it before the new entry claims the merge slot.
            std::string old = EntryText(g_visit.pendingEntry);
            acc::speech::Speak(old.c_str(), /*interrupt=*/true);
            acclog::Write("FloorPuzzle", "announce: [%s] (pending flush)",
                          old.c_str());
        }
        g_visit.pendingEntry   = n.plate;
        g_visit.pendingEntryAt = now;
    }

    std::string line;
    if (b.anyFlip) {
        if (g_visit.pendingEntry >= 0) {
            line = EntryText(g_visit.pendingEntry) + ". ";
            g_visit.pendingEntry = -1;
        } else if (!leaderCaused) {
            line = std::string(Get(Id::FloorPartyToggled)) + ": ";
        }
        line += DeltaText(b.flipped, b.lit, b.litCount);
    } else if (g_visit.pendingEntry >= 0 &&
               (now - g_visit.pendingEntryAt) >= kEntryMergeWindowMs) {
        // No toggle followed (reset plate on an already-dark board, or a
        // timing miss) — the entry still gets spoken, just alone.
        line = EntryText(g_visit.pendingEntry);
        g_visit.pendingEntry = -1;
    }
    if (line.empty()) return false;
    acc::speech::Speak(line.c_str(), /*interrupt=*/true);
    acclog::Write("FloorPuzzle",
                  "announce: [%s] (plate=%d lit=%d leaderCaused=%d)",
                  line.c_str(), n.plate, b.litCount, leaderCaused ? 1 : 0);
    return true;
}

// Off-plate stream: the nearest plate and the cardinal steps to it.
// Re-announces whenever the spoken line changes, not just the nearest
// identity (identity-only gating left walking toward one plate silent),
// and stays quiet while standing still. No separate close-range warning:
// it mostly fired redundantly with a deliberate step, and the stream's
// 1-metre floor covers the rest.
void AnnounceNearest(const Vector& pos, DWORD now) {
    int   nearest = -1;
    float nearestDist = 1e9f;
    for (int i = 0; i < kTrackCount; ++i) {
        float d = DistToPlate(g_visit.plates[i], pos.x, pos.y);
        if (d < nearestDist) { nearestDist = d; nearest = i; }
    }
    if (nearest < 0) return;

    std::string say = FullName(nearest);
    std::string axes = AxisPhrase(pos, g_visit.plates[nearest]);
    if (!axes.empty()) say += ", " + axes;
    if (say == g_visit.lastNearestLine ||
        (now - g_visit.nearestSpokenAt) < kNearestMinGapMs) {
        return;
    }
    g_visit.lastNearestLine = say;
    g_visit.nearestSpokenAt = now;
    acc::speech::Speak(say.c_str(), /*interrupt=*/true);
    acclog::Write("FloorPuzzle", "nearest: [%s] dist=%.2f", say.c_str(),
                  nearestDist);
}

}  // namespace

bool IsActive() { return g_isPuzzleArea && g_visit.cacheReady; }

bool IsPuzzlePlateTrigger(uint32_t handle) {
    if (!IsActive() || handle == 0) return false;
    for (const Plate& p : g_visit.plates) {
        if (p.trigHandle == handle) return true;
    }
    return false;
}

void Tick() {
    DWORD now = GetTickCount();
    MaybeReadBoard();

    if ((now - g_lastScan) < kScanIntervalMs) return;
    g_lastScan = now;
    if (!TrackArea()) return;

    BoardScan board = ReadBoardScan();
    if (QuietAfterSolve(board)) return;
    NavScan nav = TrackNavPlate(now);
    if (AnnounceEntryOrDelta(board, nav, now) || !nav.havePos) return;
    if (!g_visit.introSpoken && MaybeSpeakIntro(nav.pos)) return;
    if (g_visit.curPlate < 0) AnnounceNearest(nav.pos, now);
}

}  // namespace acc::floor_puzzle
