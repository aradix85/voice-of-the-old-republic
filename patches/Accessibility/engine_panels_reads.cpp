// Engine reads beside the panel registry: the CGuiInGame and main-interface
// pointers, the dialog replies, and the workbench upgrade-slot table.

#include "engine_panels_internal.h"

#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_reads.h"

namespace acc::engine {

// See the header for why the index lives here rather than at the call sites.
//
// The upper bounds are the entry counts each game's table actually carries.
// KOTOR 1's is the long-standing 16 (4 categories x 4 slots). KOTOR 2's blocks
// are 6 slots wide and a dump of the table shows four fully-populated category
// blocks plus a fifth that begins well-formed, so 30 is the evidence-backed
// bound; a category beyond that reports no-entry and the caller falls back to
// its generic label rather than reading past what has been looked at.
bool LookupUpgradeSlotType(int slotCustomValue, int category,
                           int* outUpgradeType, uint32_t* outStrRef) {
    int tableIdx;
    int tableCount;
    if (acc::game::IsKotor2()) {
        // KOTOR 2 indexes the raw slot value and biases the category.
        if (slotCustomValue < 0 || slotCustomValue > 5 || category < 1) return false;
        tableIdx   = slotCustomValue + (category - 1) * 6;
        tableCount = 30;
    } else {
        // KOTOR 1 biases the slot by four and does not bias the category.
        tableIdx   = (slotCustomValue - 4) + category * 4;
        tableCount = 16;
    }
    if (tableIdx < 0 || tableIdx >= tableCount) return false;

    int      upgradeType = -1;
    uint32_t strref      = 0;
    __try {
        auto* entry = reinterpret_cast<unsigned char*>(
            kAddrUpgradeSlotTypeTable + tableIdx * kUpgradeSlotTypeStride);
        upgradeType = *reinterpret_cast<int*>(entry);
        strref      = *reinterpret_cast<uint32_t*>(
            entry + kUpgradeSlotTypeStrRefOff);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    // Sentinel rows mark slot positions the category does not use.
    if (upgradeType == -1 || strref == 0) return false;

    if (outUpgradeType) *outUpgradeType = upgradeType;
    if (outStrRef)      *outStrRef      = strref;
    return true;
}

void* ResolveGuiInGame() {
    void* internal = GetClientAppInternal();
    return ReadPtrOrNull(internal, kClientExoAppGuiInGameOff);
}

void* ResolveMainInterface() {
    void* guiInGame = ResolveGuiInGame();
    return ReadPtrOrNull(guiInGame, kGuiInGameMainInterfaceOff);
}

bool ReadDialogReplyText(int replyIndex, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    if (replyIndex < 0) return false;
    void* gui = ResolveGuiInGame();
    if (!gui) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(gui);
        // field69_0x114 = reply array capacity/count (SetReplyData guards on it).
        uint32_t count = *reinterpret_cast<uint32_t*>(
            base + kCGuiInGameReplyCountOffset);
        if (count == 0 || count > 256) return false;  // sanity bound
        if (static_cast<uint32_t>(replyIndex) >= count) return false;
        // field70_0x118 = pointer to the CExoString[] array (8 bytes/entry).
        void* arr = *reinterpret_cast<void**>(
            base + kCGuiInGameReplyTextArrayOffset);
        if (!arr) return false;
        return ReadCExoString(arr, static_cast<size_t>(replyIndex) * 8,
                              outBuf, bufSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
        return false;
    }
}

int ReadDialogReplyCount() {
    void* gui = ResolveGuiInGame();
    int value = -1;
    TryReadI32(gui, kCGuiInGameReplyCountOffset, &value);
    return value;
}

}  // namespace acc::engine
