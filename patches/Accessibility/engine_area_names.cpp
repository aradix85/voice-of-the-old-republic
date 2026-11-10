// Object names as the mod speaks them: the authored name cleaned up, player-name tokens filled in, a fallback for unnamed objects, and the state suffix (door, lock, container) appended.

#include "engine_area_internal.h"
#include "engine_app.h"     // GetServerApp, GetClientApp
#include "engine_player.h"  // player and leader names
#include "engine_reads.h"   // ExtractTextOrStrRef, ReadCExoString

#include "strings.h"        // door state suffix lookup (DoorOpen/DoorLocked)

namespace acc::engine {

namespace {

// Strip the two kinds of authoring residue KOTOR 2's object names carry into
// speech. Both are things the module designers left in the name string itself,
// so they reach us through the ordinary LocName/tag read and there is nothing
// upstream to fix.
//
//  * A trailing {...} note. K2's doors and containers are routinely named
//    "Blast Door{HK-50}", "Emergency Blast Door{103PER}", "Medical Bay
//    Container{Chems}", "Door{Kreia Prison Warning}" — module codes, script
//    hints and disambiguators for the level designer. A few read as almost
//    useful ("Blast Door{Starboard Dormitory}") but most are noise, and read
//    aloud they are noise every single time the object is announced, including
//    inside action prompts ("Open Medical Bay Container{Chems}").
//
//  * An unsubstituted <FullName> token — the Peragus medbay tank announced as
//    "<FullName>'s Kolto Tank" (four times in the beta log). Handled by the
//    shared SubstitutePlayerNameToken above, which the message router also
//    calls; the token is not specific to object names.
void CleanAuthoredName(char* buf, size_t bufSize) {
    if (!buf || bufSize < 2 || !buf[0]) return;

    SubstitutePlayerNameToken(buf, bufSize);

    // Trailing {...}, plus any whitespace it was separated by.
    size_t len = strlen(buf);
    if (len >= 2 && buf[len - 1] == '}') {
        for (size_t i = len - 1; i-- > 0;) {
            if (buf[i] == '{') {
                while (i > 0 && (buf[i - 1] == ' ' || buf[i - 1] == '\t')) --i;
                buf[i] = '\0';
                break;
            }
        }
    }
}

}  // namespace

// <FullName> → the PC's name. Only this token is known to appear in the
// strings we read raw; anything else in angle brackets is left alone rather
// than guessed at. Shared because the token reaches us on more than one
// path — placed-object names via CleanAuthoredName below, and engine feedback
// lines via the message router (the Peragus bonus quest announces as
// "<FullName> Needs Medical Attention" on every journal update).
void SubstitutePlayerNameToken(char* buf, size_t bufSize) {
    if (!buf || bufSize < 2 || !buf[0]) return;
    const char* kToken = "<FullName>";
    const size_t kTokenLen = 10;
    char* hit = strstr(buf, kToken);
    if (!hit) return;

    // The chargen-name slot is the right source, but it comes back empty
    // on KOTOR 2 (witnessed 2026-08-13: the Peragus tank announced as
    // "'s Kolto Tank" with nothing before the possessive). Fall back to
    // the active leader, which resolves in-world on both games.
    char pcName[64] = "";
    if (!GetPlayerCharacterName(pcName, sizeof(pcName)) || pcName[0] == '\0') {
        pcName[0] = '\0';
        if (!GetActiveLeaderName(pcName, sizeof(pcName))) {
            pcName[0] = '\0';
        }
    }
    // Substituting an empty string leaves dangling grammar — "'s Kolto
    // Tank" — which is worse than the raw token,
    // because at least the token tells the listener a name is missing
    // rather than sounding like a finished sentence. Leave it alone.
    if (pcName[0] == '\0') return;

    size_t nameLen = strlen(pcName);
    size_t tailLen = strlen(hit + kTokenLen);
    size_t headLen = static_cast<size_t>(hit - buf);
    if (headLen + nameLen + tailLen + 1 <= bufSize) {
        memmove(hit + nameLen, hit + kTokenLen, tailLen + 1);
        memcpy(hit, pcName, nameLen);
    }
}

// Resolve a CExoLocString by treating its 8 bytes as the same shape as
// CExoString (char* + uint32 length); fall back to TLK strref at +4. Both
// reads are SEH-guarded by the engine_reads helpers.
bool TryReadLocString(void* base, size_t locStringOffset,
                      char* outBuf, size_t bufSize) {
    return ExtractTextOrStrRef(base, locStringOffset,
                               locStringOffset + 4, outBuf, bufSize);
}

// Last-resort: speak the modder-assigned tag instead of an empty name.
// Better "g_dnt_carth" than nothing.
bool TryReadTag(void* obj, char* outBuf, size_t bufSize) {
    __try {
        return ReadCExoString(obj, kObjectTagOffset, outBuf, bufSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Append ", <text>" to outBuf if there's room. No-op when text is empty
// or outBuf is already at capacity.
void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text) {
    if (!outBuf || bufSize < 3 || !text || !text[0]) return;
    size_t curLen = 0;
    while (curLen < bufSize && outBuf[curLen]) ++curLen;
    if (curLen >= bufSize - 1) return;
    size_t remaining = bufSize - curLen - 1;
    if (remaining < 3) return;  // no room for ", x" minimum
    outBuf[curLen++] = ',';
    outBuf[curLen++] = ' ';
    remaining -= 2;
    for (size_t i = 0; i < remaining && text[i]; ++i) {
        outBuf[curLen++] = text[i];
    }
    outBuf[curLen] = '\0';
}

// Inner: one engine call with the exact handle we got. Returns true
// on a non-empty resolved name. Empty result and engine-side faults
// fold into a single false return so the outer can retry.
static bool TryResolveDisplayNameOnce(void* clientApp, uint32_t handle,
                                      char* outBuf, size_t bufSize) {
    typedef int (__thiscall* PFN_GetObjectName)(void* this_, uint32_t handle,
                                                CExoString* outStr);
    CExoString out{nullptr, 0};
    __try {
        auto fn = reinterpret_cast<PFN_GetObjectName>(
            kAddrCClientExoAppGetObjectName);
        fn(clientApp, handle, &out);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!out.c_string || out.length == 0 || out.length >= bufSize) {
        return false;
    }
    __try {
        memcpy(outBuf, out.c_string, out.length);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
        return false;
    }
    outBuf[out.length] = '\0';
    return outBuf[0] != '\0';
}

bool GetObjectDisplayNameByHandle(uint32_t handle,
                                  char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    if (IsSentinelHandle(handle)) return false;

    void* clientApp = GetClientApp();
    if (!clientApp) return false;

    // Try the handle as-is first. Then, if that fails AND the handle
    // looked server-side (high bit clear), retry with the high bit
    // set. CClientExoApp::GetObjectName resolves only the client-side
    // namespace (verified 2026-05-10 in patch-20260510-003647.log:
    // 0x8000002c → "Sith-Soldier" succeeded; 0x0000002c → empty/tag
    // fallback). Server-side IDs with high bit cleared share their
    // low 24 bits with the matching client handle per
    // memory:project_object_handle_namespaces, so OR'ing 0x80000000
    // is the namespace-cross conversion.
    if (TryResolveDisplayNameOnce(clientApp, handle, outBuf, bufSize)) {
        return true;
    }
    if ((handle & 0x80000000u) == 0u) {
        outBuf[0] = '\0';
        if (TryResolveDisplayNameOnce(clientApp,
                                      handle | 0x80000000u,
                                      outBuf, bufSize)) {
            return true;
        }
    }
    outBuf[0] = '\0';
    return false;
}

void AppendObjectStateSuffix(void* gameObject, char* outBuf, size_t bufSize) {
    if (!gameObject || !outBuf || bufSize < 3 || outBuf[0] == '\0') return;
    const int kind = GetObjectKind(gameObject);
    size_t curLen = std::strlen(outBuf);
    if (curLen + 3 >= bufSize) return;
    if (kind == static_cast<int>(GameObjectKind::Placeable)) {
        // Locked containers (KOTOR 1): "Footlocker 2, locked, Security 12".
        bool locked = false;
        int dc = 0;
        ReadLockNeed(gameObject, dc, &locked);
        if (locked) {
            AppendCommaSeparated(outBuf, bufSize,
                acc::strings::Get(acc::strings::Id::DoorLocked));
            AppendLockNeed(gameObject, outBuf, bufSize);
        }
        return;
    }
    if (kind != static_cast<int>(GameObjectKind::Door)) return;
    BuildDoorSuffix(gameObject, outBuf + curLen, bufSize - curLen);
}

bool GetObjectName(void* gameObject, char* outBuf, size_t bufSize) {
    if (!GetObjectBaseName(gameObject, outBuf, bufSize)) return false;
    AppendObjectStateSuffix(gameObject, outBuf, bufSize);
    return outBuf[0] != '\0';
}

bool GetObjectBaseName(void* gameObject, char* outBuf, size_t bufSize) {
    if (!gameObject || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';

    int kind = GetObjectKind(gameObject);
    if (kind < 0) return false;

    using K = GameObjectKind;
    bool got = false;
    switch (K(kind)) {
        case K::Door:
            // Bare LocName only. The state ("locked"/"open"/
            // "entriegelt") + transition destination + description
            // enrichment lives in AppendObjectStateSuffix, which
            // GetObjectName applies on top — so narration can slot a
            // disambiguating number in between the two halves.
            got = TryReadLocString(gameObject, kDoorLocNameOffset,
                                   outBuf, bufSize);
            break;
        case K::Creature: {
            // Prefer the engine's universal display-name accessor (same
            // string a sighted user sees in tooltip / target reticle —
            // localized via dialog.tlk). Generic spawns like
            // tar02_woman02 / tar02_maintdrd have empty first_name
            // strrefs, so the per-stats path below would otherwise fall
            // through to the raw tag. Verified in combat.cpp /
            // combat_queue.cpp where the same accessor returns
            // "Sith-Soldier" etc.
            uint32_t handle = GetObjectHandle(gameObject);
            if (handle != 0u &&
                GetObjectDisplayNameByHandle(handle, outBuf, bufSize) &&
                outBuf[0] != '\0') {
                got = true;
                break;
            }
            outBuf[0] = '\0';
            __try {
                void* stats = *reinterpret_cast<void**>(
                    reinterpret_cast<unsigned char*>(gameObject) +
                    kCreatureStatsPtrOffset);
                if (stats) {
                    got = TryReadLocString(
                        stats, kCreatureStatsFirstNameOffset,
                        outBuf, bufSize);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                got = false;
            }
            break;
        }
        case K::Placeable:
            got = TryReadLocString(gameObject, kPlaceableLocNameOffset,
                                   outBuf, bufSize);
            break;
        case K::Item:
            got = TryReadLocString(gameObject, kItemLocNameOffset,
                                   outBuf, bufSize);
            if (!got || outBuf[0] == '\0') {
                // Same fallback the Creature case uses, for the same reason:
                // an item dropped in the world usually carries no LocName of
                // its own (the displayed name comes from its template / base
                // item), so this fell through to the raw tag and announced a
                // resref — "g_w_sbrcrstl11" instead of a lightsaber crystal
                // (patch-20260813-150242, spoken eight times). The engine's
                // universal accessor gives the string a sighted player reads
                // in the tooltip. Falls through to the tag as before when it
                // has nothing either.
                uint32_t handle = GetObjectHandle(gameObject);
                outBuf[0] = '\0';
                got = handle != 0u &&
                      GetObjectDisplayNameByHandle(handle, outBuf, bufSize) &&
                      outBuf[0] != '\0';
            }
            if (!got) {
                // A mod item named only in another language (the engine
                // never crosses languages) — its real name beats the tag.
                got = ReadLocStringOtherLanguage(gameObject, kItemLocNameOffset,
                                                 outBuf, bufSize);
            }
            break;
        case K::Waypoint:
            // For a landmark waypoint the curated map-note label (+0x230)
            // IS the canonical name — the exact string sighted players
            // read off the area map, and what the map-context cycle
            // already prioritises. Prefer it outright: stock K1 leaves the
            // waypoint LocName (+0x238) empty but sets a resref-style Tag
            // ("k35_map_dreshdae"), so a LocName-first order would let that
            // machine name win in world context. Non-landmark waypoints
            // have no map note; GetWaypointMapNote returns false and we
            // fall through to LocName → tag.
            if (GetWaypointMapNote(gameObject, outBuf, bufSize)) {
                got = true;
                break;
            }
            got = TryReadLocString(gameObject, kWaypointLocNameOffset,
                                   outBuf, bufSize);
            break;
        case K::Trigger:
            got = TryReadLocString(gameObject, kTriggerLocNameOffset,
                                   outBuf, bufSize);
            // Area-transition triggers carry their human-readable "to X"
            // label in the transition_destination LocString (CSWSTrigger
            // +0x30c), exactly like doors (BuildDoorSuffix reads the door
            // equivalent). The trigger LocName above is near-always empty
            // in stock K1, so without this the Transition cycle category
            // falls through to the raw tag. Non-transition triggers (trap
            // / encounter) have an empty transition_destination and still
            // fall to the tag below.
            if (!got || outBuf[0] == '\0') {
                got = TryReadLocString(gameObject, kTriggerTransitionDestOffset,
                                       outBuf, bufSize);
            }
            break;
        default:
            return false;
    }

    if (got && outBuf[0] != '\0') {
        CleanAuthoredName(outBuf, bufSize);
        return outBuf[0] != '\0';
    }
    if (!TryReadTag(gameObject, outBuf, bufSize)) return false;
    CleanAuthoredName(outBuf, bufSize);
    return outBuf[0] != '\0';
}

}  // namespace acc::engine
