// Item and creature reads on top of the generic readers of engine_reads.cpp:
// client/server handle translation and item resolution, stack, charge and
// inventory-row reads, item names in another language, the workbench slots
// and a creature's Force points. Item descriptions are in
// engine_reads_items_desc.cpp, action descriptions in _actions.cpp; all are
// declared in engine_reads.h.

#include "engine_reads_items_internal.h"

#include "engine_area.h"    // GetObjectDisplayNameByHandle
#include "engine_offsets_select.h"
#include "log.h"

namespace acc::engine {

namespace {

typedef uint32_t (__thiscall* PFN_ClientToServerObjectId)(void* this_, uint32_t handle);
typedef void*    (__thiscall* PFN_GetItemByGameObjectID)(void* this_, uint32_t handle);

// The English dialog.tlk LanguageID: the `language` half of a CExoLocString
// substring id.
constexpr uint32_t kEnglish = 0;

uintptr_t ReadVtable(void* control) {
    void* vt = nullptr;
    if (!control || !TryReadPtr(control, 0, &vt)) return 0;
    return reinterpret_cast<uintptr_t>(vt);
}

bool IsItemEntryRow(void* control) {
    const uintptr_t v = ReadVtable(control);
    return v != 0 && (v == kVtableCSWGuiInGameItemEntry || v == kVtableCSWGuiStoreItemEntry);
}

// The obj_id the row was built for (same offset on both row classes); 0 when
// the control is no row or the read faults.
uint32_t ReadItemRowHandle(void* rowControl) {
    uint32_t handle = 0;
    if (IsItemEntryRow(rowControl)) {
        TryReadU32(rowControl, kStoreItemEntryObjIdOffset, &handle);
    }
    return handle;
}

// Charged items (max_charges > 0) cannot stack; false for any other item.
bool ReadItemChargesRaw(void* item, int* outCharges) {
    uint32_t charges = 0, maxCharges = 0;
    if (!item || !TryReadU32(item, kSwsItemChargesOffset, &charges) ||
        !TryReadU32(item, kSwsItemMaxChargesOffset, &maxCharges)) {
        return false;
    }
    if (maxCharges == 0) return false;
    if (outCharges) *outCharges = static_cast<int>(charges);
    return true;
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

void* ResolveItemFromServerHandle(uint32_t serverHandle) {
    if (serverHandle == 0 || serverHandle == 0xffffffff) return nullptr;
    void* serverApp = GetServerApp();
    if (!serverApp) return nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetItemByGameObjectID>(
            kAddrServerExoAppGetItemByGameObjectID);
        return fn(serverApp, serverHandle);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void* ResolveItemFromClientHandle(uint32_t clientHandle) {
    return ResolveItemFromServerHandle(ClientToServerObjectId(clientHandle));
}

// CSWSItem.stack_size; 0 on a fault and for infinite-stock items (the store
// reads that flag itself).
int ReadItemStack(void* item) {
    uint32_t bitFlags = 0;
    uint16_t stack = 0;
    if (!item || !TryReadU32(item, kSwsItemBitFlagsOffset, &bitFlags) ||
        !TryReadU16(item, kSwsItemStackSizeOffset, &stack)) {
        return 0;
    }
    return (bitFlags & kSwsItemInfiniteStockBit) ? 0 : static_cast<int>(stack);
}

int ReadItemRowStackCount(void* rowControl) {
    return ReadItemStack(ResolveItemFromClientHandle(ReadItemRowHandle(rowControl)));
}

int ReadItemCharges(void* item) {
    int charges = -1;
    return ReadItemChargesRaw(item, &charges) ? charges : -1;
}

int ReadItemRowCharges(void* rowControl) {
    void* item = ResolveItemFromClientHandle(ReadItemRowHandle(rowControl));
    return item ? ReadItemCharges(item) : -1;
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
    if (first && ReadCExoString(first, kExoLocStringEntryTextOffset, outBuf, bufSize)) {
        return true;
    }
    outBuf[0] = '\0';
    return false;
}

bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    const uint32_t handle = ReadItemRowHandle(rowControl);
    if (handle == 0) return false;
    // The engine's localised name accessor crosses the client/server handle
    // namespaces itself; the row stores a client handle.
    if (GetObjectDisplayNameByHandle(handle, outBuf, bufSize) && outBuf[0] != '\0') {
        return true;
    }
    // A mod item named only in another language reads empty above (the engine
    // never crosses languages), as the K2 mod robes a_robe_21 / a_robe_22 did.
    void* item = ResolveItemFromClientHandle(handle);
    if (item && ReadLocStringOtherLanguage(item, kItemLocNameOffset, outBuf, bufSize)) {
        acclog::Trace("Menus.ItemRow",
                      "row=%p handle=0x%08x named from another-language "
                      "substring: \"%s\"", rowControl, handle, outBuf);
        return true;
    }
    acclog::Write("Menus.ItemRow", "row=%p handle=0x%08x name-resolve missed",
                  rowControl, handle);
    outBuf[0] = '\0';
    return false;
}

bool IsInventoryItemRow(void* control) {
    const uintptr_t v = ReadVtable(control);
    return v != 0 && v == kVtableCSWGuiInGameItemEntry;
}

void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl) {
    if (!upgradePanel || !slotControl) return nullptr;
    uint32_t customValue = 0;
    if (!TryReadU32(slotControl, kUpgradeSlotCustomValueOff, &customValue)) return nullptr;
    // One installed-mod pointer per slot: a real slot button holds 0..3.
    if (customValue > 3) return nullptr;
    void* item = nullptr;
    TryReadPtr(upgradePanel, kUpgradeSlotInstalledItemsOff + customValue * 4, &item);
    return item;
}

WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel) {
    WorkbenchPickerInfo info;
    void* slotBtn = nullptr;
    if (!upgradePanel || !TryReadPtr(upgradePanel, kUpgradeActiveSlotOff, &slotBtn) ||
        !slotBtn) {
        return info;
    }
    uint32_t customValue = 0;
    if (!TryReadU32(slotBtn, kUpgradeSlotCustomValueOff, &customValue)) return info;

    info.valid = true;
    if (customValue == 1) {
        // The saber colour slot: row 0 is the current crystal, every row a
        // real choice, no remove entry.
        info.isColorSlot  = true;
        info.minSel       = 0;
        info.installedRow = 0;
    } else {
        // Other slots: row 0 is the remove entry, row 1 the installed crystal
        // when there is one.
        info.minSel       = 1;
        info.installedRow = GetWorkbenchSlotInstalledItem(upgradePanel, slotBtn) ? 1 : -1;
    }
    return info;
}

}  // namespace acc::engine
