// item / creature domain readers.
//
// Split out of engine_reads.cpp by the Phase-1 structure pass (refactoring
// candidate 4). engine_reads.cpp kept the generic GUI-control readers
// (control text, tooltips, TLK lookup, toggle/slider/listbox predicates);
// this file owns the domain layer that grew on top of them:
//
//   * client/server handle translation and item resolution
//   * feat / spell / action description resolvers
//   * item stack, charge and inventory-row reads
//   * the item property-description block reconstruction
//   * creature Force-point reads
//
// Declarations stay in engine_reads.h - splitting that header is a
// separate, deferred item, so every existing includer is unaffected.

#include "engine_reads_items_internal.h"

#include "engine_area.h"    // GetObjectDisplayNameByHandle
#include "log.h"
#include "engine_offsets_select.h"

namespace acc::engine {

namespace {

typedef uint32_t (__thiscall* PFN_ClientToServerObjectId)(void* this_,
                                                           uint32_t handle);
typedef void*    (__thiscall* PFN_GetItemByGameObjectID)(void* this_,
                                                         uint32_t handle);

// The English dialog.tlk LanguageID: the `language` half of a CExoLocString
// substring id.
constexpr uint32_t kEnglish = 0;

// Read CSWSItem.stack_size (2 bytes) and bit_flags (4 bytes). Mirrors the
// store-side ReadItemStock helper but doesn't return the infinite-stock
// flag separately — callers that need that distinction use the store
// path. Returns 0 on fault or infinite-stock items.
int ReadItemStackSize(void* item) {
    if (!item) return 0;
    uint32_t bitFlags = 0;
    uint16_t stack = 0;
    __try {
        bitFlags = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(item) + kSwsItemBitFlagsOffset);
        stack = *reinterpret_cast<uint16_t*>(
            reinterpret_cast<unsigned char*>(item) + kSwsItemStackSizeOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    if (bitFlags & kSwsItemInfiniteStockBit) return 0;
    return (int)stack;
}

// Read CSWSItem.charges (+0x258) when the item is a charged item
// (max_charges @ +0x25c > 0). Returns the current charge count via
// *outCharges and true; returns false (item not charge-based / fault)
// otherwise. Charged items can't stack, so this is orthogonal to
// ReadItemStackSize.
bool ReadItemChargesRaw(void* item, int* outCharges) {
    if (!item) return false;
    uint32_t charges = 0, maxCharges = 0;
    __try {
        charges = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(item) + kSwsItemChargesOffset);
        maxCharges = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(item) + kSwsItemMaxChargesOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (maxCharges == 0) return false;
    if (outCharges) *outCharges = (int)charges;
    return true;
}

bool IsItemEntryRow(void* control) {
    if (!control) return false;
    void** vt = nullptr;
    __try {
        vt = *reinterpret_cast<void***>(control);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    auto v = reinterpret_cast<uintptr_t>(vt);
    return v == kVtableCSWGuiInGameItemEntry ||
           v == kVtableCSWGuiStoreItemEntry;
}

// obj_id the row was built for. Same offset on both row vtables. 0 when the
// control is not a row or the read faults, which every caller treats as
// "nothing to resolve".
uint32_t ReadItemRowHandle(void* rowControl) {
    if (!IsItemEntryRow(rowControl)) return 0;
    __try {
        return *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(rowControl) +
            kStoreItemEntryObjIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

}  // namespace

uint32_t ClientToServerObjectId(uint32_t clientHandle) {
    if (clientHandle == 0 || clientHandle == 0xffffffff) return 0;
    void* serverApp = GetServerApp();
    if (!serverApp) return 0;

    uint32_t serverHandle = 0;
    __try {
        auto fn = reinterpret_cast<PFN_ClientToServerObjectId>(
            kAddrServerExoAppClientToServerObjectId);
        serverHandle = fn(serverApp, clientHandle);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    if (serverHandle == 0 || serverHandle == 0xffffffff) return 0;
    return serverHandle;
}

void* ResolveItemFromClientHandle(uint32_t clientHandle) {
    uint32_t serverHandle = ClientToServerObjectId(clientHandle);
    if (serverHandle == 0) return nullptr;

    void* serverApp = GetServerApp();
    if (!serverApp) return nullptr;

    void* item = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetItemByGameObjectID>(
            kAddrServerExoAppGetItemByGameObjectID);
        item = fn(serverApp, serverHandle);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return item;
}

void* ResolveItemFromServerHandle(uint32_t serverHandle) {
    if (serverHandle == 0 || serverHandle == 0xffffffff) return nullptr;
    void* serverApp = GetServerApp();
    if (!serverApp) return nullptr;

    void* item = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetItemByGameObjectID>(
            kAddrServerExoAppGetItemByGameObjectID);
        item = fn(serverApp, serverHandle);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return item;
}

int ReadItemRowStackCount(void* rowControl) {
    uint32_t handle = ReadItemRowHandle(rowControl);
    if (handle == 0) return 0;
    void* item = ResolveItemFromClientHandle(handle);
    if (!item) return 0;
    return ReadItemStackSize(item);
}

int ReadItemCharges(void* item) {
    int charges = -1;
    return ReadItemChargesRaw(item, &charges) ? charges : -1;
}

int ReadItemRowCharges(void* rowControl) {
    uint32_t handle = ReadItemRowHandle(rowControl);
    if (handle == 0) return -1;
    void* item = ResolveItemFromClientHandle(handle);
    if (!item) return -1;
    return ReadItemCharges(item);
}

bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset,
                                char* outBuf, size_t bufSize) {
    if (!base || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    const uint32_t kNoId = 0xFFFFFFFF;

    // Entries, not text: the choice needs the whole list seen first (a
    // English substring anywhere means the engine's own text stands).
    void* first = nullptr;
    __try {
        void* internal = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(base) + locStringOffset +
            kExoLocStringInternalOffset);
        if (!internal) return false;
        void* list = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internal) + kListInternalOffset);
        if (!list) return false;
        void* node = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(list) + kListInternalHeadOffset);
        // A loc string holds at most one substring per language and gender
        // (six languages x 2); the bound only stops a corrupt cycle.
        for (int guard = 0; node && guard < 32; ++guard) {
            void* entry = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(node) + kLinkedListNodeDataOff);
            if (entry) {
                const uint32_t id = *reinterpret_cast<uint32_t*>(
                    reinterpret_cast<unsigned char*>(entry) +
                    kExoLocStringEntryIdOffset);
                if (id != kNoId && (id >> 1) == kEnglish) return false;
                if (!first) first = entry;
            }
            node = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(node) + kLinkedListNodeNextOff);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    // ReadCExoString is SEH-guarded itself and false on empty text.
    if (first &&
        ReadCExoString(first, kExoLocStringEntryTextOffset, outBuf, bufSize)) {
        return true;
    }
    outBuf[0] = '\0';
    return false;
}

bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    uint32_t handle = ReadItemRowHandle(rowControl);
    if (handle == 0) return false;
    // Engine's universal name accessor - localised, and it crosses the
    // client/server handle namespaces itself, which matters here because
    // the row stores a CLIENT handle while most of our name reads start
    // from a server object.
    if (!GetObjectDisplayNameByHandle(handle, outBuf, bufSize) ||
        outBuf[0] == '\0') {
        // A mod item whose name exists only in another language resolves
        // empty through the accessor above (the engine never crosses
        // languages). Store rows showed "control 3" for the K2 mod robes
        // a_robe_21 / a_robe_22, English-only (patch-20260917-112203.log).
        void* item = ResolveItemFromClientHandle(handle);
        if (item && ReadLocStringOtherLanguage(item, kItemLocNameOffset,
                                               outBuf, bufSize)) {
            acclog::Trace("Menus.ItemRow",
                          "row=%p handle=0x%08x named from another-language "
                          "substring: \"%s\"", rowControl, handle, outBuf);
            return true;
        }
        acclog::Write("Menus.ItemRow",
                      "row=%p handle=0x%08x name-resolve missed",
                      rowControl, handle);
        outBuf[0] = '\0';
        return false;
    }
    return true;
}

int ReadItemStack(void* item) {
    return ReadItemStackSize(item);
}

bool IsInventoryItemRow(void* control) {
    if (!control) return false;
    void** vt = nullptr;
    __try {
        vt = *reinterpret_cast<void***>(control);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return reinterpret_cast<uintptr_t>(vt) == kVtableCSWGuiInGameItemEntry;
}

void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl) {
    if (!upgradePanel || !slotControl) return nullptr;
    void* item = nullptr;
    __try {
        int customValue = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(slotControl) +
            kUpgradeSlotCustomValueOff);
        // Guard the index — the array holds at most 4 mod pointers (one per
        // slot). custom_value should always be 0..3 for a real slot button.
        if (customValue < 0 || customValue > 3) return nullptr;
        item = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(upgradePanel) +
            kUpgradeSlotInstalledItemsOff + customValue * 4);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads",
                      "GetWorkbenchSlotInstalledItem SEH panel=%p slot=%p",
                      upgradePanel, slotControl);
        return nullptr;
    }
    return item;
}

WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel) {
    WorkbenchPickerInfo info;
    if (!upgradePanel) return info;

    void* slotBtn = nullptr;
    int customValue = -1;
    __try {
        slotBtn = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(upgradePanel) +
            kUpgradeActiveSlotOff);
        if (slotBtn) {
            customValue = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(slotBtn) +
                kUpgradeSlotCustomValueOff);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads",
                      "GetWorkbenchPickerInfo SEH panel=%p", upgradePanel);
        return info;
    }
    if (!slotBtn) return info;

    info.valid = true;
    if (customValue == 1) {
        // Saber color slot: row 0 is the current colour crystal, every row is
        // a real choice, no remove entry.
        info.isColorSlot  = true;
        info.minSel       = 0;
        info.installedRow = 0;
    } else {
        // Power / non-color saber slot: row 0 is the 0x7f000000 remove entry;
        // row 1 is the installed crystal when the slot is occupied.
        info.minSel       = 1;
        void* installed = GetWorkbenchSlotInstalledItem(upgradePanel, slotBtn);
        info.installedRow = installed ? 1 : -1;
    }
    return info;
}

bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax) {
    if (outCur) *outCur = 0;
    if (outMax) *outMax = 0;
    if (!clientCreature) return false;

    // CSWCCreature+0x2f8 -> CSWCLevelUpStats* (embeds CSWCCreatureStats).
    // Same chain as combat_query's HP reads; force pool lives further into
    // the same struct.
    //
    //   +0x11e  short  max_force_points   (cached computed max — LIVE)
    //   +0x120  short  force_points       (BASE term fed into GetMaxForcePoints,
    //                                       NOT the live current pool — ignore)
    //   +0x122  short  field85_0x122  ┐ live current force = sum of these two,
    //   +0x124  short  field86_0x124  ┘ exactly as the engine's own
    //                                   CSWGuiInGameCharacter::SetStats
    //                                   (@0x006afda0) computes the FP label off
    //                                   this client struct. (Note the server
    //                                   GetCurrentForcePoints sums +0x124/+0x126
    //                                   — the server stats struct is shifted 2
    //                                   bytes from this client one, so reuse of
    //                                   the server offsets here reads garbage.)
    // KOTOR 2 values from its charsheet SetStats twin 0x0084E6F0 (the FP
    // label writer): stats root at clientCreature+0x310 (`creature[0xc4]`
    // dword — double-witnessed, Batch 3d used the same root for HP), max FP
    // = short [stats+0x126], current FP = (short)([stats+0x12a] +
    // [stats+0x12c]) behind the same IsJedi-style class gate. The FP band
    // shifted +8; HP (+0x4c/+0x4e) did not — bands move independently.
    const size_t kLvlUpStatsOffset = acc::off::Pick(0x2f8, 0x310);
    const size_t kMaxForceOffset   = acc::off::Pick(0x11e, 0x126);
    const size_t kCurForceLoOffset = acc::off::Pick(0x122, 0x12a);
    const size_t kCurForceHiOffset = acc::off::Pick(0x124, 0x12c);

    void* lvlUpStats = nullptr;
    __try {
        lvlUpStats = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(clientCreature) +
            kLvlUpStatsOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!lvlUpStats) return false;

    __try {
        unsigned char* stats = reinterpret_cast<unsigned char*>(lvlUpStats);
        int maxFp = static_cast<int>(
            *reinterpret_cast<short*>(stats + kMaxForceOffset));
        // Match the engine: (short)(field_0x124 + field_0x126).
        int curFp = static_cast<int>(static_cast<short>(
            *reinterpret_cast<short*>(stats + kCurForceLoOffset) +
            *reinterpret_cast<short*>(stats + kCurForceHiOffset)));
        if (outMax) *outMax = maxFp;
        if (outCur) *outCur = curFp;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return true;
}

}  // namespace acc::engine
