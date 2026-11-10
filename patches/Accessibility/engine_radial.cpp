// The engine's target action menu (CSWGuiTargetActionMenu): its rows, their
// action lists and selection, and dispatching a row's action.

#include "engine_radial_internal.h"

#include "engine_offsets.h"
#include "engine_app.h"
#include "engine_panels.h"
#include "log.h"

namespace acc::engine_radial {

namespace {

// CSWGuiTargetActionMenu lives inside CSWGuiMainInterface at +0xBC. Verified
// against the C header (swkotor.exe.h:11611 MEMBER OFFSET="0xbc" SIZE="0x1af0").
// K2 listing-witnessed in the K2 CSWGuiMainInterface::PopulateMenus
// (0x0074CFE0): the TAM::PopulateMenus call receiver is `this + 0xCC`.
const size_t kMainInterfaceTargetActionMenuOffset = acc::off::Pick(0xBC, 0xCC);

// CSWGuiLabel name_label sits at TAM +0x15CC (relative). Reading via
// kLabelGuiStringPtrOffset (0xE4) for the rendered c_string.
// K2 witnessed in the K2 TAM ctor: first gui_string label constructed right
// after the position-field block (K1 0x15bc..0x15c8 -> K2 0x1644..0x1664),
// at this+0x1668 (further labels at +0x17b0/+0x18f8, stride 0x148).
const size_t kTamNameLabelOffset      = acc::off::Pick(0x15CC, 0x1668);

// field1[12] = 4 target_types × 3 rows; each int is the selected action_id
// for that combination (-1 = use first item). Used by inner-PopulateMenus
// + SelectNext/Prev/DoTargetAction to mark the active action in
// action_lists[row]. Source-of-truth for "which descriptor is selected
// right now"; the rendered target_actions[r].action_button only catches
// up after the engine's next paint pass.
// K2: selected-id 2-D array stays at +0x24; the target-type byte moved
// 0x1AEA -> 0x1BAA (both witnessed in the K2 DoTargetAction).
const size_t kTamField1Offset         = acc::off::Same(0x24);
const size_t kTamTargetTypeOffset     = acc::off::Pick(0x1AEA, 0x1BAA);
constexpr int    kActionIdNone            = -1;
const size_t kIfActionStride          = acc::off::Pick(0x38, 0x3c);

// Engine entry points (k1_win_gog_swkotor.exe.xml symbol table).
// K2 twin found by the six-StrRef "can't do that" switch shared with
// DoPersonalAction; disambiguated by its TAM-layout reads (lists at +0,
// selected array +0x24, target-type byte +0x1baa).
const uintptr_t kAddrDoTargetAction = acc::addr::Pick(0x00689610, 0x007445D0);

bool ValidRow(int row) { return row >= 0 && row < kRowCount; }

// action_lists[row]: its descriptor array and size. Raw reads; the caller
// holds the SEH frame.
struct ActionList {
    unsigned char* entries;
    int32_t        size;
};

ActionList ReadActionList(unsigned char* tam, int row) {
    size_t listOff = kTamActionListsOffset + row * kActionListStride;
    return {*reinterpret_cast<unsigned char**>(tam + listOff),
            *reinterpret_cast<int32_t*>(tam + listOff + kActionListSizeOffset)};
}

unsigned char* Entry(const ActionList& list, int index) {
    return list.entries + index * kIfActionStride;
}

int32_t EntryId(const ActionList& list, int index) {
    return *reinterpret_cast<int32_t*>(Entry(list, index) + kIfActionIdOffset);
}

// The selected-action slot for this row under the menu's current target
// type, or nullptr when the type is out of range. Raw read; the caller holds
// the SEH frame.
int32_t* SelectedIdSlot(unsigned char* tam, int row) {
    uint8_t targetType = *(tam + kTamTargetTypeOffset);
    if (targetType >= 4) return nullptr;
    return reinterpret_cast<int32_t*>(
        tam + kTamField1Offset +
        (static_cast<size_t>(targetType) * 3 + row) * sizeof(int32_t));
}

void* RowActionButtonAddr(void* tam, int row) {
    if (!tam || !ValidRow(row)) return nullptr;
    return reinterpret_cast<unsigned char*>(tam) + kTamTargetActionsOffset +
           row * kTargetActionStride + kRowActionButtonOffset;
}

// The descriptor in action_lists[row] whose id is the engine's selected
// one. data[0] when nothing is selected (-1) or no entry has the stored id,
// as the engine's SelectNextAction lookup does; nullptr for an empty row or
// a fault.
void* FindSelectedActionDescriptor(void* tam, int row) {
    if (!tam || !ValidRow(row)) return nullptr;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(tam);
        ActionList list = ReadActionList(base, row);
        if (!list.entries || list.size <= 0) return nullptr;

        int32_t* slot = SelectedIdSlot(base, row);
        int32_t selectedId = slot ? *slot : kActionIdNone;
        if (selectedId != kActionIdNone) {
            for (int i = 0; i < list.size; ++i) {
                if (EntryId(list, i) == selectedId) return Entry(list, i);
            }
        }
        return list.entries;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

}  // namespace

void* ResolveTargetActionMenu() {
    void* mainIf = acc::engine::ResolveMainInterface();
    if (!mainIf) return nullptr;
    return reinterpret_cast<unsigned char*>(mainIf) +
           kMainInterfaceTargetActionMenuOffset;
}

int RowActionCount(void* tam, int row) {
    if (!tam || !ValidRow(row)) return 0;
    int32_t size = 0;
    size_t offset = kTamActionListsOffset + row * kActionListStride +
                    kActionListSizeOffset;
    if (!ReadInt32(tam, offset, &size)) return 0;
    return size < 0 ? 0 : static_cast<int>(size);
}

uint32_t ReadSelectedRowActionId(void* tam, int row) {
    int32_t id = 0;
    if (!ReadInt32(FindSelectedActionDescriptor(tam, row), kIfActionIdOffset, &id)) {
        return 0;
    }
    return static_cast<uint32_t>(id);
}

bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    void* btn = RowActionButtonAddr(tam, row);
    if (!btn) return false;

    // The rendered text first; the engine sometimes leaves gui_string null
    // between SelectNextAction calls, so then the inline CExoString.
    if (acc::engine::ReadGuiString(btn, kButtonGuiStringPtrOffset, outBuf, bufSize)) {
        return true;
    }
    if (acc::engine::ReadCExoString(btn, kButtonTextOffset, outBuf, bufSize)) {
        return true;
    }
    // Right after PopulateMenus the button is still empty until the next
    // paint pass; the selected descriptor holds the label it will show.
    void* descriptor = FindSelectedActionDescriptor(tam, row);
    return descriptor &&
           acc::engine::ReadCExoString(descriptor, kIfActionLabelOffset,
                                       outBuf, bufSize);
}

bool ReadTargetName(void* tam, char* outBuf, size_t bufSize) {
    if (!tam || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    void* nameLabel = reinterpret_cast<unsigned char*>(tam) + kTamNameLabelOffset;
    return acc::engine::ReadGuiString(nameLabel, kLabelGuiStringPtrOffset,
                                      outBuf, bufSize) ||
           acc::engine::ReadCExoString(nameLabel, kLabelTextOffset, outBuf, bufSize);
}

bool DispatchRowAction(void* tam, int row) {
    if (!tam || !ValidRow(row)) return false;
    __try {
        auto fn = reinterpret_cast<PFN_DoTargetAction>(kAddrDoTargetAction);
        fn(tam, row, 0);  // trailing 0: DoTargetAction purges 8 (ret 8)
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Radial", "DispatchRowAction SEH-FAULT row=%d", row);
        return false;
    }
}

bool SelectActionInRow(void* tam, int row, int index) {
    if (!tam || !ValidRow(row) || index < 0) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(tam);
        ActionList list = ReadActionList(base, row);
        if (!list.entries || index >= list.size) return false;

        int32_t actionId = EntryId(list, index);
        if (actionId == 0) return false;

        int32_t* slot = SelectedIdSlot(base, row);
        if (!slot) return false;
        *slot = actionId;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Radial", "SelectActionInRow SEH-FAULT row=%d idx=%d",
                      row, index);
        return false;
    }
}

uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index) {
    if (!tam || !ValidRow(row) || index < 0) return 0;
    __try {
        ActionList list = ReadActionList(reinterpret_cast<unsigned char*>(tam), row);
        if (!list.entries || index >= list.size) return 0;
        return static_cast<uint32_t>(EntryId(list, index));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

int FindRowIndexByActionId(void* tam, int row, uint32_t actionId) {
    if (!tam || !ValidRow(row) || actionId == 0) return -1;
    __try {
        ActionList list = ReadActionList(reinterpret_cast<unsigned char*>(tam), row);
        if (!list.entries) return -1;
        for (int i = 0; i < list.size; ++i) {
            if (static_cast<uint32_t>(EntryId(list, i)) == actionId) return i;
        }
        return -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle) {
    if (!tam || !ValidRow(row)) return -1;
    __try {
        ActionList list = ReadActionList(reinterpret_cast<unsigned char*>(tam), row);
        if (!list.entries || list.size <= 0) return -1;

        uint32_t prev = *reinterpret_cast<uint32_t*>(
            Entry(list, 0) + kIfActionTargetOffset);
        for (int i = 0; i < list.size; ++i) {
            *reinterpret_cast<uint32_t*>(Entry(list, i) + kIfActionTargetOffset) =
                targetClientHandle;
        }
        if (prev != targetClientHandle) {
            acclog::Write("Radial",
                "retarget row=%d stale creature_id 0x%08x -> 0x%08x "
                "(%d entries)", row, prev, targetClientHandle, list.size);
        }
        return list.size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Radial", "RetargetRowActions SEH-FAULT row=%d", row);
        return -1;
    }
}

bool IsCreatureClientTarget(uint32_t handle) {
    if (handle == 0u || handle == 0xFFFFFFFFu || handle == kInvalidObjectId) {
        return false;
    }
    uint32_t client = handle | 0x80000000u;  // engine convention: client high bit
    void* exoApp = acc::engine::GetClientApp();
    if (!exoApp) return false;
    void* gameObject = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetGameObject>(kAddrCClientExoAppGetGameObject);
        gameObject = fn(exoApp, client);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!gameObject) return false;
    return CallVtableAsClass(gameObject, kVtableAsSWCCreatureOffset) != nullptr;
}

}  // namespace acc::engine_radial
