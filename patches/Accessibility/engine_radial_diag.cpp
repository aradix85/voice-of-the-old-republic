// Diagnostic dumps of the target action menu and of a target's door preconditions for the log.

#include "engine_radial_internal.h"

#include "engine_offsets.h"
#include "engine_app.h"
#include "engine_player.h"
#include "log.h"

namespace acc::engine_radial {

namespace {

// CSWCCreatureStats::GetCanUseSkill @ 0x006477e0 — bool __thiscall
// (CSWCCreatureStats*, ushort skill_idx). Used by CSWCDoor::GetTargetActions
// to gate the Security row (skill_idx=6 = Security).
// K2 twin: the K2 GetDefaultActions mine case calls it (push 1) exactly
// where KOTOR 1's calls 0x006477E0. Receiver chain (lvl-up stats offset)
// listing-witnessed separately.
const uintptr_t kAddrGetCanUseSkill = acc::addr::Pick(0x006477E0, 0x00845730);

// CSWSDoor.field_0x2d8 — the second Security gate from CSWCDoor::Get-
// TargetActions decomp: must be 0 for the Security row to populate.
// Per the door decomp, the field is read as `*(int *)(server_door + 0x2d8)`
// and compared `== 0`.
// K2 witnessed in the K2 CSWCDoor::GetTargetActions (0x0087BEC0) Security
// case: `GetServerDoor()+0x328 == 0 && GetCanUseSkill(6)` gates the
// Security row (id 0x3f3) — K1's shape with the gate moved 0x2D8 -> 0x328.
const size_t kServerDoorSecurityGateOffset = acc::off::Pick(0x2D8, 0x328);

// CSWCDoor field offsets (swkotor.exe.h:6192 STRUCTURE NAME="CSWCDoor"):
//   +0x104  cannot_bash       (int) — gates Bash row
//   +0x108  can_use_actions   (int) — gates BOTH Bash and Security rows
//                                     (per CSWCDoor::GetTargetActions decomp)
//   +0x114  is_hostile        (int)
//   +0x11c  state             (int) — door open/closed state
//   +0x138  field17           (int) — additional Bash-row gate
// K2 client-door deltas are PIECEWISE (witnessed in the K2
// GetDefaultActions door case): cannot_bash stays +0x104, can_use_actions
// moved +0x108 -> +0x10c, field17 +0x138 -> +0x13c — i.e. one dword
// inserted between +0x104 and +0x108. is_hostile/state sit in the shifted
// region and follow the +4 delta pending their own decompile witness.
const size_t kDoorCannotBashOffset     = acc::off::Same(0x104);
const size_t kDoorCanUseActionsOffset  = acc::off::Pick(0x108, 0x10c);
// is_hostile/state sit inside the (0x108..0x138] range whose +4 shift is
// bracketed by direct witnesses on BOTH ends (0x108->0x10c, 0x138->0x13c),
// so the single-insertion model fixes them at +4. Diag-only reads
// (LogTargetDiag) — a wrong value logs wrong, it cannot crash (SEH).
const size_t kDoorIsHostileOffset      = acc::off::Pick(0x114, 0x118);
const size_t kDoorStateOffset          = acc::off::Pick(0x11c, 0x120);
const size_t kDoorField17Offset        = acc::off::Pick(0x138, 0x13c);

// Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer
// as NUL-terminated within the 16-byte window.
void ReadResRef(void* base, size_t offset, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return;
    outBuf[0] = '\0';
    if (!base) return;
    __try {
        const char* src = reinterpret_cast<const char*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        size_t lim = bufSize - 1;
        if (lim > kResRefSize) lim = kResRefSize;
        size_t i = 0;
        for (; i < lim; ++i) {
            char c = src[i];
            outBuf[i] = c;
            if (c == '\0') return;
        }
        outBuf[i] = '\0';
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
    }
}

}  // namespace

void LogState(void* tam, const char* tag) {
    if (!tam) {
        acclog::Write("Radial.State", "[%s] tam=NULL", tag ? tag : "?");
        return;
    }

    // Hex dump the first 0x40 bytes of TAM (covers all 3 action_lists +
    // half of field1[12]). Eight uint32 per line.
    char hex[160];
    for (int line = 0; line < 4; ++line) {
        size_t off = line * 0x10;
        uint32_t w[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; ++i) {
            int32_t tmp = 0;
            TryReadI32(tam, off + i * 4, &tmp);
            w[i] = static_cast<uint32_t>(tmp);
        }
        std::snprintf(hex, sizeof(hex),
                      "[%s] tam+%02x: %08x %08x %08x %08x",
                      tag ? tag : "?", static_cast<unsigned>(off),
                      w[0], w[1], w[2], w[3]);
        acclog::Write("Radial.State", "%s", hex);
    }

    // Parsed action_lists.
    for (int r = 0; r < kRowCount; ++r) {
        size_t base = kTamActionListsOffset + r * kActionListStride;
        int32_t dataPtr = 0, size = 0, cap = 0;
        TryReadI32(tam, base + 0x00, &dataPtr);
        TryReadI32(tam, base + 0x04, &size);
        TryReadI32(tam, base + 0x08, &cap);
        acclog::Write("Radial.State", "[%s] action_lists[%d] data=0x%08x size=%d cap=%d",
            tag ? tag : "?", r,
            static_cast<uint32_t>(dataPtr), size, cap);
    }

    // Per-row is_action + action_button.text length (via gui_string).
    for (int r = 0; r < kRowCount; ++r) {
        size_t rowBase = kTamTargetActionsOffset + r * kTargetActionStride;
        int32_t isAction = 0;
        TryReadI32(tam, rowBase + kRowIsActionOffset, &isAction);

        char label[128] = "";
        void* btn = reinterpret_cast<unsigned char*>(tam) + rowBase +
                    kRowActionButtonOffset;
        acc::engine::ReadGuiString(btn, kButtonGuiStringPtrOffset,
                                   label, sizeof(label));
        size_t labelLen = 0;
        for (; labelLen < sizeof(label) && label[labelLen]; ++labelLen) {}

        acclog::Write("Radial.State", "[%s] target_actions[%d] is_action=%d "
            "action_button.gui_string=[%s] (len=%zu)",
            tag ? tag : "?", r, isAction, label, labelLen);
    }

    char nameBuf[64] = "";
    ReadTargetName(tam, nameBuf, sizeof(nameBuf));
    acclog::Write("Radial.State", "[%s] name_label.text=[%s]",
                  tag ? tag : "?", nameBuf);
}

void LogStateWide(void* tam, const char* tag) {
    // Re-use the standard dump as the prefix so log readers don't have to
    // chase two separate snapshot blocks for the same call.
    LogState(tam, tag);
    if (!tam) return;

    // field1[12] — 12 ints starting at TAM +0x24. Hypothesis: per-row
    // selected-action index per mode, with -1 meaning "no selection".
    {
        int32_t f[12] = {0};
        for (int i = 0; i < 12; ++i) {
            TryReadI32(tam, 0x24 + i * 4, &f[i]);
        }
        acclog::Write("Radial.StateW", "[%s] field1[0..3]=%d,%d,%d,%d "
            "[4..7]=%d,%d,%d,%d [8..11]=%d,%d,%d,%d",
            tag ? tag : "?",
            f[0], f[1], f[2], f[3],
            f[4], f[5], f[6], f[7],
            f[8], f[9], f[10], f[11]);
    }

    // For each row: full button text dump + field4/field5 + is_action.
    for (int r = 0; r < kRowCount; ++r) {
        size_t rowBase = kTamTargetActionsOffset + r * kTargetActionStride;
        unsigned char* row =
            reinterpret_cast<unsigned char*>(tam) + rowBase;
        char actBtn[96]   = "";
        char actLbl[96]   = "";
        char upBtn[96]    = "";
        char downBtn[96]  = "";
        acc::engine::ReadButtonText(row + kRowActionButtonOffset, actBtn, sizeof(actBtn));
        acc::engine::ReadButtonText(row + kRowActionLabelOffset,  actLbl, sizeof(actLbl));
        acc::engine::ReadButtonText(row + kRowUpButtonOffset,     upBtn,  sizeof(upBtn));
        acc::engine::ReadButtonText(row + kRowDownButtonOffset,   downBtn, sizeof(downBtn));
        int32_t f4 = 0, f5 = 0, isAct = 0;
        TryReadI32(tam, rowBase + kRowField4Offset, &f4);
        TryReadI32(tam, rowBase + kRowField5Offset, &f5);
        TryReadI32(tam, rowBase + kRowIsActionOffset, &isAct);

        acclog::Write("Radial.StateW", "[%s] target_actions[%d] is_action=%d "
            "field4=0x%08x field5=0x%08x",
            tag ? tag : "?", r, isAct,
            static_cast<uint32_t>(f4), static_cast<uint32_t>(f5));
        acclog::Write("Radial.StateW", "[%s] target_actions[%d] action_button=[%s] "
            "action_label=[%s]",
            tag ? tag : "?", r, actBtn, actLbl);
        acclog::Write("Radial.StateW", "[%s] target_actions[%d] up_button=[%s] "
            "down_button=[%s]",
            tag ? tag : "?", r, upBtn, downBtn);
    }

    // Peek action_lists[r].data[0] when the engine reserved capacity but
    // didn't update size — maybe the action descriptor was written but
    // the size update was skipped, leaving us a recoverable label.
    for (int r = 0; r < kRowCount; ++r) {
        size_t base = kTamActionListsOffset + r * kActionListStride;
        int32_t dataPtr = 0, sizeF = 0, cap = 0;
        TryReadI32(tam, base + 0x00, &dataPtr);
        TryReadI32(tam, base + 0x04, &sizeF);
        TryReadI32(tam, base + 0x08, &cap);
        if (cap <= 0 || dataPtr == 0) continue;
        void* peek = reinterpret_cast<void*>(static_cast<uintptr_t>(dataPtr));

        char    label[64] = "";
        char    icon[16]  = "";
        int32_t actionId = 0;
        int32_t targetId = 0;
        acc::engine::ReadCExoString(peek, kIfActionLabelOffset, label, sizeof(label));
        TryReadI32(peek, kIfActionIdOffset,    &actionId);
        TryReadI32(peek, kIfActionTargetOffset, &targetId);
        ReadResRef(peek, kIfActionIconOffset, icon, sizeof(icon));

        acclog::Write("Radial.StateW", "[%s] action_lists[%d].data[0] peek "
            "label=[%s] action_id=0x%x target_id=0x%08x icon=[%s] "
            "(size=%d cap=%d data=0x%08x)",
            tag ? tag : "?", r, label,
            static_cast<uint32_t>(actionId),
            static_cast<uint32_t>(targetId), icon,
            sizeF, cap, static_cast<uint32_t>(dataPtr));
    }
}

void LogTargetDiag(uint32_t targetClient, const char* tag) {
    const char* t = tag ? tag : "?";

    if (targetClient == 0u || targetClient == 0xFFFFFFFFu ||
        targetClient == kInvalidObjectId) {
        acclog::Write("Radial.Diag", "[%s] target=0x%08x — sentinel/invalid",
                      t, targetClient);
        return;
    }

    void* exoApp = acc::engine::GetClientApp();
    if (!exoApp) {
        acclog::Write("Radial.Diag", "[%s] target=0x%08x — no client exo app",
                      t, targetClient);
        return;
    }

    void* gameObject = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetGameObject>(
            kAddrCClientExoAppGetGameObject);
        gameObject = fn(exoApp, targetClient);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        gameObject = nullptr;
    }
    if (!gameObject) {
        acclog::Write("Radial.Diag", "[%s] target=0x%08x — GetGameObject returned NULL",
            t, targetClient);
        return;
    }

    // Read the vtable address itself for log identification — different
    // runtime classes have different vtables, so this is a quick "is the
    // door we got the same one between calls" check.
    uintptr_t vtableAddr = 0;
    __try {
        vtableAddr = *reinterpret_cast<uintptr_t*>(gameObject);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        vtableAddr = 0;
    }

    void* asDoor      = CallVtableAsClass(gameObject, kVtableAsSWCDoorOffset);
    void* asCreature  = CallVtableAsClass(gameObject, kVtableAsSWCCreatureOffset);
    void* asPlaceable = CallVtableAsClass(gameObject, kVtableAsSWCPlaceableOffset);
    void* asTrigger   = CallVtableAsClass(gameObject, kVtableAsSWCTriggerOffset);

    const char* kind = "unknown";
    if      (asDoor)      kind = "door";
    else if (asCreature)  kind = "creature";
    else if (asPlaceable) kind = "placeable";
    else if (asTrigger)   kind = "trigger";

    acclog::Write("Radial.Diag", "[%s] target=0x%08x gameObject=%p vtable=0x%08x kind=%s "
        "(door=%p creature=%p placeable=%p trigger=%p)",
        t, targetClient, gameObject,
        static_cast<uint32_t>(vtableAddr), kind,
        asDoor, asCreature, asPlaceable, asTrigger);

    if (asDoor) {
        int32_t cannotBash = -1;
        int32_t canUseActions = -1;
        int32_t isHostile = -1;
        int32_t state = -1;
        int32_t field17 = -1;
        TryReadI32(asDoor, kDoorCannotBashOffset,    &cannotBash);
        TryReadI32(asDoor, kDoorCanUseActionsOffset, &canUseActions);
        TryReadI32(asDoor, kDoorIsHostileOffset,     &isHostile);
        TryReadI32(asDoor, kDoorStateOffset,         &state);
        TryReadI32(asDoor, kDoorField17Offset,       &field17);
        acclog::Write("Radial.Diag", "[%s] door=%p cannot_bash=%d can_use_actions=%d "
            "is_hostile=%d state=%d field17=%d",
            t, asDoor, cannotBash, canUseActions, isHostile, state, field17);

        // Server-side door's Security gate (kServerDoorSecurityGateOffset)
        // must be 0 for CSWCDoor::GetTargetActions to add the Security row.
        // The server object sits at the same offset on every client object.
        void* serverDoor = nullptr;
        __try {
            serverDoor = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(asDoor) +
                kClientObjectServerObjectOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            serverDoor = nullptr;
        }
        int32_t securityGate = -1;
        bool gateRead = false;
        if (serverDoor) {
            gateRead = TryReadI32(serverDoor,
                                 kServerDoorSecurityGateOffset, &securityGate);
        }
        if (gateRead) {
            acclog::Write("Radial.Diag", "[%s] server_door=%p security_gate=%d "
                "(must be 0 to add Security row)",
                t, serverDoor, securityGate);
        } else {
            acclog::Write("Radial.Diag", "[%s] server_door=%p — security_gate read failed",
                t, serverDoor);
        }

        // Active leader's Security skill check — same call CSWCDoor::Get-
        // TargetActions makes: GetCanUseSkill(&leader->lvl_up_stats->
        // creature_stats, 6). lvl_up_stats is a pointer at leader+0x2f8;
        // CSWCCreatureStats is the inline first member of CSWCLevelUpStats
        // so the pointer can be passed straight to GetCanUseSkill.
        void* clientLeader = acc::engine::GetClientLeader();
        int leaderSecurity = -1;
        if (clientLeader) {
            void* lvlUpStats = nullptr;
            __try {
                lvlUpStats = *reinterpret_cast<void**>(
                    reinterpret_cast<unsigned char*>(clientLeader) +
                    kClientCreatureLevelUpStatsOffset);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                lvlUpStats = nullptr;
            }
            if (lvlUpStats) {
                __try {
                    auto fn = reinterpret_cast<PFN_GetCanUseSkill>(
                        kAddrGetCanUseSkill);
                    leaderSecurity = fn(lvlUpStats,
                                        static_cast<uint16_t>(kSkillSecurity))
                                     ? 1 : 0;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    leaderSecurity = -1;
                }
            }
        }
        acclog::Write("Radial.Diag", "[%s] leader=%p has_security_skill=%d "
            "(skill 6, gates the Security row in CSWCDoor::GetTargetActions)",
            t, clientLeader, leaderSecurity);

        // Annotate which precondition would fail — saves cross-referencing
        // the GetTargetActions decomp every time a log lands.
        if (canUseActions == 0) {
            acclog::Write("Radial.Diag", "[%s] door precondition fail — can_use_actions=0 "
                "(door already in final state, e.g. opened); both Bash and "
                "Security rows skipped by CSWCDoor::GetTargetActions",
                t);
        }
        if (cannotBash != 0) {
            acclog::Write("Radial.Diag", "[%s] door precondition fail — cannot_bash=%d "
                "(scripted door, no Bash row even when can_use_actions != 0)",
                t, cannotBash);
        }
        if (field17 != 0) {
            acclog::Write("Radial.Diag", "[%s] door precondition fail — field17=%d "
                "(blocks Bash row regardless of cannot_bash / can_use_actions)",
                t, field17);
        }
        if (gateRead && securityGate != 0) {
            acclog::Write("Radial.Diag", "[%s] door precondition fail — server_door security_gate "
                "= %d != 0 (door rejects Security row at the server-side gate)",
                t, securityGate);
        }
        if (canUseActions != 0 && leaderSecurity == 0) {
            acclog::Write("Radial.Diag", "[%s] door precondition fail — active leader "
                "lacks Security skill (skill 6); Security row skipped even "
                "when door's preconditions allow it",
                t);
        }
    }
}

}  // namespace acc::engine_radial
