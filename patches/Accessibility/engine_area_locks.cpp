// What it takes to open a locked door or container: a key, the Security DC,
// and who in the party has the best Security.

#include "engine_area_internal.h"

#include "engine_game.h"    // IsKotor1: the lock fields are KOTOR 1 offsets
#include "engine_offsets_select.h"
#include "engine_reads.h"   // ReadCExoString
#include "log.h"
#include "party_status.h"   // best Security in the party
#include "strings.h"        // the lock need words

namespace acc::engine {

namespace {

struct LockFields {
    int locked      = 0;
    int keyRequired = 0;
    int dc          = 0;
};

// One SEH-guarded read of an object's lock fields. The two flags are GFF
// BYTEs stored in wider slots, so only their low byte is read.
bool ReadLockFieldsRaw(void* obj, size_t lockedOff, size_t keyReqOff,
                       size_t dcOff, bool dcIsShort, LockFields& out) {
    __try {
        auto* base = reinterpret_cast<unsigned char*>(obj);
        out.locked      = *(base + lockedOff);
        out.keyRequired = *(base + keyReqOff);
        out.dc = dcIsShort ? *reinterpret_cast<short*>(base + dcOff)
                           : static_cast<int>(*(base + dcOff));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// One log line per object (recent handles remembered), so the raw values
// can be checked against the object's UTD / UTP template.
void LogLockOnce(void* obj, int kind, const LockFields& f, const char* key,
                 LockNeed need, const char* failScript = "") {
    static uint32_t s_logged[64] = {0};
    static int      s_next = 0;
    const uint32_t handle = GetObjectHandle(obj);
    for (uint32_t h : s_logged) {
        if (h == handle) return;
    }
    s_logged[s_next] = handle;
    s_next = (s_next + 1) % 64;
    char tag[64] = {0};
    GetObjectTag(obj, tag, sizeof(tag));
    acclog::Write("LockNeed",
                  "tag=%s kind=%d locked=%d keyRequired=%d dc=%d key='%s' "
                  "failToOpen='%s' -> %d",
                  tag, kind, f.locked, f.keyRequired, f.dc, key ? key : "",
                  failScript ? failScript : "", static_cast<int>(need));
}

}  // namespace

// ", opens only through the story" / ", needs a key" / ", Security 15"
// after the "locked" word, so the player knows whether to bring a key, a
// slicer, or nothing at all (KOTOR 1; see ReadLockNeed).
void AppendLockNeed(void* obj, char* outBuf, size_t bufSize) {
    int dc = 0;
    switch (ReadLockNeed(obj, dc)) {
        case LockNeed::StoryOnly:
            AppendCommaSeparated(outBuf, bufSize,
                acc::strings::Get(acc::strings::Id::LockNeedStoryOnly));
            break;
        case LockNeed::TryIt:
            AppendCommaSeparated(outBuf, bufSize,
                acc::strings::Get(acc::strings::Id::LockNeedTryIt));
            break;
        case LockNeed::Key:
            AppendCommaSeparated(outBuf, bufSize,
                acc::strings::Get(acc::strings::Id::LockNeedKey));
            break;
        case LockNeed::Security: {
            char buf[64];
            std::snprintf(buf, sizeof(buf),
                acc::strings::Get(acc::strings::Id::FmtLockNeedSecurity), dc);
            AppendCommaSeparated(outBuf, bufSize, buf);
            std::string who;
            int score = 0;
            if (acc::party_status::BestSecurity(who, score)) {
                char best[192];
                std::snprintf(best, sizeof(best),
                    acc::strings::Get(acc::strings::Id::FmtLockBestSecurity),
                    who.c_str(), score);
                AppendCommaSeparated(outBuf, bufSize, best);
            }
            break;
        }
        default:
            break;
    }
}

LockNeed ReadLockNeed(void* gameObject, int& outDc, bool* outLocked) {
    outDc = 0;
    if (outLocked) *outLocked = false;
    if (!gameObject) return LockNeed::Unknown;
    const int kind = GetObjectKind(gameObject);
    size_t lockedOff = 0, keyReqOff = 0, keyNameOff = 0, dcOff = 0;
    bool dcIsShort = false;
    if (kind == static_cast<int>(GameObjectKind::Door)) {
        lockedOff  = kDoorLockedOffset;
        keyReqOff  = kDoorKeyRequiredOffset;
        keyNameOff = kDoorKeyNameOffset;
        dcOff      = kDoorOpenLockDcOffset;
    } else if (kind == static_cast<int>(GameObjectKind::Placeable)) {
        lockedOff  = kPlaceableLockedOffset;
        keyReqOff  = kPlaceableKeyRequiredOffset;
        keyNameOff = kPlaceableKeyNameOffset;
        dcOff      = kPlaceableOpenLockDcOffset;
        dcIsShort  = true;
    } else {
        return LockNeed::Unknown;
    }
    if (!acc::game::IsKotor1() || !acc::off::Ok(lockedOff) ||
        !acc::off::Ok(keyReqOff) || !acc::off::Ok(keyNameOff) ||
        !acc::off::Ok(dcOff)) {
        return LockNeed::Unknown;
    }
    LockFields f;
    if (!ReadLockFieldsRaw(gameObject, lockedOff, keyReqOff, dcOff, dcIsShort,
                           f)) {
        return LockNeed::Unknown;
    }
    if (f.locked == 0) {
        // Containers: log the raw fields once anyway, so the placeable
        // offsets can be checked against unlocked UTPs too.
        if (kind == static_cast<int>(GameObjectKind::Placeable)) {
            LogLockOnce(gameObject, kind, f, "", LockNeed::Unknown);
        }
        return LockNeed::Unknown;
    }
    if (outLocked) *outLocked = true;

    char key[64] = {0};
    const bool haveKey =
        ReadCExoString(gameObject, keyNameOff, key, sizeof(key)) && key[0];
    LockNeed need = LockNeed::Unknown;
    if (f.keyRequired != 0) {
        need = haveKey ? LockNeed::Key : LockNeed::StoryOnly;
    } else if (f.dc > 0 && f.dc < 100) {
        need  = LockNeed::Security;
        outDc = f.dc;
    }
    // A door no key or slicer opens but with an OnFailToOpen script: the
    // attempt is the story's cue (Endar Spire bridge doors, the Taris Upper
    // City Cantina entrance), so "opens only through the story" would talk
    // the player out of the one thing that works. Say to try it instead.
    char failScript[32] = {0};
    if (kind == static_cast<int>(GameObjectKind::Door) &&
        (need == LockNeed::StoryOnly || need == LockNeed::Unknown) &&
        acc::off::Ok(kDoorOnFailToOpenOffset) &&
        ReadCExoString(gameObject, kDoorOnFailToOpenOffset, failScript,
                       sizeof(failScript)) &&
        failScript[0]) {
        need = LockNeed::TryIt;
    }
    LogLockOnce(gameObject, kind, f, haveKey ? key : "", need, failScript);
    return need;
}

}  // namespace acc::engine
