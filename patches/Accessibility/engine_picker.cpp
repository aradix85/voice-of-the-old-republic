// The engine's own default action for a target: build its action
// descriptor the way a mouse hover does, read it, and dispatch it the way a
// click does, or open the radial menu when there is none (engine_picker.h).

#include "engine_picker.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

#include "engine_app.h"         // GetClientApp, GetClientAppInternal
#include "engine_area.h"        // ResolveClientObject
#include "engine_offsets.h"
#include "engine_offsets_select.h"
#include "engine_panels.h"      // ResolveGuiInGame, ResolveMainInterface
#include "engine_player.h"      // SetPlayerInputEnabled, GetActiveLeaderName
#include "engine_radial.h"      // diagnostics around the radial fallback
#include "engine_reads.h"
#include "engine_addr.h"
#include "log.h"

namespace {

using acc::engine::TryReadPtr;
using acc::engine::TryReadU32;
using acc::picker::ActionSnapshot;

// CClientExoAppInternal fields (struct in docs/llm-docs/re/swkotor.exe.h,
// confirmed by the decompiles of HandleMouseClickInWorld and
// GetDefaultActions):
//   last_target, last_clicked_on_target: client-side target ids
//   hover target: must equal last_target for the click gate to dispatch
//   descriptor array (CSWGuiInterfaceAction*) and count: the default action;
//     the count must be > 0 to dispatch
// KOTOR 2 values from the hover-update twin (0x007B3A00) and the click twin
// (0x007B3CA0): the hover id moved to +0x4b0 and the descriptor pair
// shifted by 4.
const size_t kInternalLastTargetOffset          = acc::off::Same(0x2b4);
const size_t kInternalLastClickedOnTargetOffset = acc::off::Same(0x2b8);
const size_t kInternalHoverTargetOffset         = acc::off::Pick(0x4a4, 0x4b0);
const size_t kInternalDescriptorArrayOffset     = acc::off::Pick(0x4c8, 0x4cc);
const size_t kInternalDescriptorCountOffset     = acc::off::Pick(0x4cc, 0x4d0);

// CSWGuiInterfaceAction fields (the writes in GetDefaultActions; struct in
// swkotor.exe.h). Same positions in KOTOR 2, witnessed across its
// GetDefaultActions writes and both Do*Action bodies. The stride (0x38,
// K2 0x3c) and the fn pointer at +0x0c are not needed here.
const size_t kInterfaceActionLabelOffset  = acc::off::Same(0x00);  // CExoString
const size_t kInterfaceActionIdOffset     = acc::off::Same(0x08);  // ulong
const size_t kInterfaceActionTargetOffset = acc::off::Same(0x1c);  // ulong
const size_t kInterfaceActionIconOffset   = acc::off::Same(0x20);  // CResRef
constexpr size_t kResRefMaxLen = 16;

// K2 GetDefaultActions was found by its icon-resref string xrefs (the only
// function referencing all six of i_noaction, i_dialog, i_opendoor,
// i_useplace, i_disablemine and i_recovermine); K2 HandleMouseClickInWorld
// by decompile: the same three gates, click-gate triple and dispatch, right
// before GetDefaultActions as in KOTOR 1.
const uintptr_t kAddrGetDefaultActions = acc::addr::Pick(0x00620620, 0x007B40F0);
const uintptr_t kAddrHandleMouseClickInWorld = acc::addr::Pick(0x00620350, 0x007B3CA0);
const uintptr_t kAddrSetMainInterfaceTarget = acc::addr::Pick(0x0062b000, 0x007CE710);
// CSWGuiMainInterface::PopulateMenus builds the radial action menu;
// HandleMouseClickInWorld calls it on the first click on a target.
const uintptr_t kAddrPopulateMenus = acc::addr::Pick(0x00689d80, 0x0074CFE0);

typedef void (__thiscall* PFN_ThisOnly)(void* this_);
// SetMainInterfaceTarget cleans TWO dwords off the stack (ret 8) although
// the decompiler shows one parameter. Pushing one corrupted the caller's
// frame; the second is unused padding.
typedef void (__thiscall* PFN_SetMainInterfaceTarget)(void* this_, uint32_t target,
                                                     uint32_t pad);
// KOTOR 2's PopulateMenus takes an int the engine always passes as 1 (full
// repopulate) and cleans it; the argless KOTOR 1 shape would under-push.
typedef void (__thiscall* PFN_PopulateMenusK2)(void* this_, int mode);

bool CallPopulateMenus(void* mainIf) {
    __try {
        if (acc::game::IsKotor2()) {
            reinterpret_cast<PFN_PopulateMenusK2>(kAddrPopulateMenus)(mainIf, 1);
        } else {
            reinterpret_cast<PFN_ThisOnly>(kAddrPopulateMenus)(mainIf);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Points the main interface at the target and has the engine build its
// action descriptor for it. SetMainInterfaceTarget takes the CGuiInGame.
bool BuildDescriptor(void* guiIn, void* internal, uint32_t targetClient) {
    __try {
        reinterpret_cast<PFN_SetMainInterfaceTarget>(kAddrSetMainInterfaceTarget)(
            guiIn, targetClient, 0u);
        reinterpret_cast<PFN_ThisOnly>(kAddrGetDefaultActions)(internal);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CallHandleMouseClickInWorld(void* internal) {
    __try {
        reinterpret_cast<PFN_ThisOnly>(kAddrHandleMouseClickInWorld)(internal);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void WriteUInt32(void* base, size_t offset, uint32_t value) {
    __try {
        *reinterpret_cast<uint32_t*>(static_cast<unsigned char*>(base) + offset) = value;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// Copies a NUL-terminated string of at most `limit` bytes, truncated to the
// buffer. "" on a null source or a fault.
void CopyCString(const char* src, size_t limit, char* out, size_t outCap) {
    out[0] = '\0';
    if (!src) return;
    __try {
        size_t i = 0;
        for (; i + 1 < outCap && i < limit && src[i] != '\0'; ++i) out[i] = src[i];
        out[i] = '\0';
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out[0] = '\0';
    }
}

// The engine's id for a handle from either namespace: server ids have the
// high bit clear, the cursor system uses the OR'd client form. 0 for no
// object.
uint32_t ToClientHandle(uint32_t handle) {
    if (handle == 0u || handle == 0xFFFFFFFFu || handle == kInvalidObjectId) return 0u;
    return handle | 0x80000000u;
}

// The descriptor the engine last built. valid only with a count above 0.
void SnapshotDescriptor(void* internal, ActionSnapshot* snap) {
    std::memset(snap, 0, sizeof(*snap));
    void*    action = nullptr;
    uint32_t count = 0;
    if (!TryReadPtr(internal, kInternalDescriptorArrayOffset, &action) ||
        !TryReadU32(internal, kInternalDescriptorCountOffset, &count)) {
        return;
    }
    snap->count = static_cast<int>(count);
    if (!action || snap->count <= 0) return;

    TryReadU32(action, kInterfaceActionIdOffset, &snap->action_id);
    TryReadU32(action, kInterfaceActionTargetOffset, &snap->target_id);
    void* label = nullptr;
    TryReadPtr(action, kInterfaceActionLabelOffset, &label);
    CopyCString(static_cast<const char*>(label), sizeof(snap->label), snap->label,
                sizeof(snap->label));
    CopyCString(static_cast<const char*>(action) + kInterfaceActionIconOffset,
                kResRefMaxLen, snap->icon, sizeof(snap->icon));
    snap->valid = true;
}

// Drops a descriptor that is not a usable action on this target.
void RejectUnusable(ActionSnapshot& snap, uint32_t targetClient) {
    if (!snap.valid) return;
    // The engine's "nothing to do here" verb, label "No Action", is what it
    // fills in for an object it will not let you interact with. Taken as an
    // action it was spoken as a verb, dispatched and then reported as failed.
    if (snap.action_id == acc::picker::kActionIdNoop) {
        acclog::Write("Picker", "descriptor is the engine noop verb (action_id=0x%x "
                      "label=[%s] target=0x%08x): treating as no default action",
                      snap.action_id, snap.label, snap.target_id);
        snap.valid = false;
        return;
    }
    // GetDefaultActions builds into a fixed slot, and when it has nothing to
    // say about the new target (a waypoint: not clickable) it leaves the
    // PREVIOUS target's descriptor there, which looks valid. The dispatch
    // trusts the descriptor's target, so Enter on a landmark once opened a
    // conversation with a party member. Conclusive only when our handle
    // names a real object: some callers arrive with a value from the wrong
    // namespace, and there the descriptor's own target is the better answer.
    if (snap.target_id && snap.target_id != targetClient &&
        acc::engine::ResolveClientObjectHandle(targetClient)) {
        acclog::Write("Picker", "descriptor belongs to 0x%08x, not 0x%08x (label=[%s] "
                      "action_id=0x%x): engine did not rebuild for this target; "
                      "treating as no default action",
                      snap.target_id, targetClient, snap.label, snap.action_id);
        snap.valid = false;
    }
}

// No default action, or the radial was asked for: open the radial menu the
// way the first mouse click on a target does, through the PopulateMenus
// wrapper. The wrapper resolves the player and the target (with the
// CGameObject to CSWCObject downcast that direct calls to the inner
// PopulateMenus lacked), refills the six personal-action lists and the
// target-action lists, and updates the UI rows.
bool OpenRadial(void* mainIf, uint32_t targetClient, int descriptorCount,
                bool forceRadial) {
    // The currently controlled party member, and the target's class and
    // state: tells an exhausted target (open door) from one the engine
    // should have offered something for (locked door, skill mismatch).
    char leaderName[64] = "";
    acc::engine::GetActiveLeaderName(leaderName, sizeof(leaderName));
    acclog::Write("Picker", "%s diag: leader=[%s] target=0x%08x descriptor_count=%d",
                  forceRadial ? "force-radial" : "empty-descriptor",
                  leaderName[0] ? leaderName : "?", targetClient, descriptorCount);
    acc::engine_radial::LogTargetDiag(targetClient, "before-wrapper");

    const bool opened = CallPopulateMenus(mainIf);
    if (void* tam = acc::engine_radial::ResolveTargetActionMenu()) {
        acc::engine_radial::LogStateWide(tam, "after-wrapper");
    }
    acclog::Write("Picker", "%s: %s (target=0x%08x descriptor_count=%d)",
                  forceRadial ? "force-radial" : "empty descriptor",
                  opened ? "opened radial via PopulateMenus" : "PopulateMenus SEH-FAULT",
                  targetClient, descriptorCount);
    return opened;
}

// Dispatches the descriptor the way a mouse click does.
bool DispatchClick(void* internal, uint32_t targetClient, const ActionSnapshot& snap) {
    // The click gate: HandleMouseClickInWorld dispatches (instead of opening
    // the radial) only when these three hold the same valid id. Written
    // directly; the engine's setters add side flags.
    WriteUInt32(internal, kInternalLastTargetOffset, targetClient);
    WriteUInt32(internal, kInternalLastClickedOnTargetOffset, targetClient);
    WriteUInt32(internal, kInternalHoverTargetOffset, targetClient);

    // Player input goes off for the action the engine enqueues, so the
    // per-tick movement input does not clobber it; TickPlayerInputRestore
    // gives it back when the queue drains. NOT for the walk-to-act verbs
    // (talk, use, bash, door, disable mine): the engine's own walk-then-act
    // only moves the PC with input left ENABLED, and disabling it froze
    // distant talks and silently failed distant loots
    // (docs/llm-docs/interaction-dispatch-model.md). The interact feature
    // arms its own approach watchdog for those.
    const bool inputDisabled = !acc::picker::IsWalkToActVerb(snap.action_id) &&
                               acc::engine::SetPlayerInputEnabled(false);
    const bool dispatched = CallHandleMouseClickInWorld(internal);
    if (!dispatched && inputDisabled) acc::engine::SetPlayerInputEnabled(true);
    acclog::Write("Picker", "HandleMouseClickInWorld %s target=0x%08x action_id=0x%x "
                  "label=[%s] (input_disabled=%d)",
                  dispatched ? "dispatched" : "SEH-FAULT", targetClient, snap.action_id,
                  snap.label, inputDisabled ? 1 : 0);
    return dispatched;
}

}  // namespace

namespace acc::picker {

bool Drive(uint32_t targetServerHandle, ActionSnapshot* outSnapshot,
           bool forceRadial, bool populateOnly) {
    ActionSnapshot snap = {};
    if (outSnapshot) *outSnapshot = snap;
    const uint32_t targetClient = ToClientHandle(targetServerHandle);
    if (!targetClient) return false;

    void* internal = acc::engine::GetClientAppInternal();
    void* guiIn    = acc::engine::ResolveGuiInGame();
    void* mainIf   = acc::engine::ResolveMainInterface();
    if (!internal || !guiIn || !mainIf) {
        acclog::Write("Picker", "chain unresolved (target=0x%08x exoApp=%p internal=%p "
                      "guiIn=%p mainIf=%p)", targetClient, acc::engine::GetClientApp(),
                      internal, guiIn, mainIf);
        return false;
    }
    if (!BuildDescriptor(guiIn, internal, targetClient)) {
        acclog::Write("Picker", "SetMainInterfaceTarget/GetDefaultActions SEH-FAULT "
                      "target=0x%08x", targetClient);
        return false;
    }
    SnapshotDescriptor(internal, &snap);
    RejectUnusable(snap, targetClient);

    if (!snap.valid || forceRadial) {
        snap.radial_opened = OpenRadial(mainIf, targetClient, snap.count, forceRadial);
        if (outSnapshot) *outSnapshot = snap;
        return snap.radial_opened;
    }
    if (outSnapshot) *outSnapshot = snap;
    acclog::Write("Picker", "descriptor populated target=0x%08x action_id=0x%x "
                  "label=[%s] icon=[%s] count=%d%s",
                  targetClient, snap.action_id, snap.label, snap.icon, snap.count,
                  populateOnly ? " (populate-only; caller dispatches)" : "");
    // populate-only: the caller picks the dispatch primitive per verb.
    if (populateOnly) return true;
    return DispatchClick(internal, targetClient, snap);
}

// Drive's radial branch without the diagnostics: it runs on every radial
// keypress. Rebuilds the target-action menu for our target, over whatever
// the cursor's last mouse move left there.
bool ReanchorRadial(uint32_t targetServerHandle) {
    const uint32_t targetClient = ToClientHandle(targetServerHandle);
    void* internal = acc::engine::GetClientAppInternal();
    void* guiIn    = acc::engine::ResolveGuiInGame();
    void* mainIf   = acc::engine::ResolveMainInterface();
    if (!targetClient || !internal || !guiIn || !mainIf) return false;

    if (!BuildDescriptor(guiIn, internal, targetClient) || !CallPopulateMenus(mainIf)) {
        acclog::Write("Picker", "ReanchorRadial SEH-FAULT target=0x%08x", targetClient);
        return false;
    }
    return true;
}

bool ReadCurrent(ActionSnapshot* outSnapshot) {
    ActionSnapshot snap = {};
    SnapshotDescriptor(acc::engine::GetClientAppInternal(), &snap);
    if (outSnapshot) *outSnapshot = snap;
    return snap.valid;
}

// CSWCCreature::ActionInitiateDialog, `this` = the target NPC's client
// creature. Its two stack parameters are unused but callee-cleaned (ret 8),
// so they must be pushed. The K2 twin is the fn pointer K2's
// GetDefaultActions stores into the talk descriptor (id 0x3ea, "i_dialog"),
// the slot KOTOR 1 stores 0x0060f620 into; its ret count is unverified.
const uintptr_t kAddrActionInitiateDialog = acc::addr::Pick(0x0060f620, 0x0077CF70);
typedef void (__thiscall* PFN_ActionInitiateDialog)(void* this_, uint32_t unused1,
                                                    void* unused2);

bool InitiateDialog(uint32_t targetServerHandle) {
    const uint32_t clientHandle = ToClientHandle(targetServerHandle);
    if (!clientHandle) return false;
    void* creature = acc::engine::ResolveClientObject(clientHandle);
    if (!creature) {
        acclog::Write("Picker", "InitiateDialog: client creature unresolved "
                      "(target=0x%08x)", clientHandle);
        return false;
    }
    __try {
        reinterpret_cast<PFN_ActionInitiateDialog>(kAddrActionInitiateDialog)(
            creature, 0u, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Picker", "InitiateDialog: SEH-FAULT target=0x%08x creature=%p",
                      clientHandle, creature);
        return false;
    }
    acclog::Write("Picker", "InitiateDialog dispatched (target=0x%08x creature=%p)",
                  clientHandle, creature);
    return true;
}

}  // namespace acc::picker
