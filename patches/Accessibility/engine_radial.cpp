
#include "engine_radial_internal.h"

#include "engine_offsets.h"   // kButtonGuiStringPtrOffset,
                              // kButtonTextOffset, kLabelGuiStringPtrOffset,
                              // kLabelTextOffset, kAurGuiStringCStrOffset,
                              // kVtableCAurGUIStringInternal, CExoString
#include "engine_app.h"       // GetClientApp
#include "engine_panels.h"    // ResolveGuiInGame, ResolveMainInterface
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
// up after the engine's next paint pass, which is why we sometimes see
// `gui_string=[] action_button=[]` immediately after PopulateMenus even
// though the data is fully populated.
// K2: selected-id 2-D array stays at +0x24; the target-type byte moved
// 0x1AEA -> 0x1BAA (both witnessed in the K2 DoTargetAction).
const size_t kTamField1Offset         = acc::off::Same(0x24);
const size_t kTamTargetTypeOffset     = acc::off::Pick(0x1AEA, 0x1BAA);
constexpr int    kActionIdNone            = -1;
const size_t kIfActionStride          = acc::off::Pick(0x38, 0x3c);

// Engine entry points (k1_win_gog_swkotor.exe.xml symbol table).
// K2 twin found by the six-StrRef "can't do that" switch shared with
// DoPersonalAction; disambiguated by its TAM-layout reads (lists at +0,
// selected array +0x24, target-type byte +0x1baa). NOTE K2 ret 8 —
// verify the K1 typedef's arg count against it before clearing gates.
const uintptr_t kAddrDoTargetAction = acc::addr::Pick(0x00689610, 0x007445D0);

void* RowActionAddr(void* tam, int row) {
    if (!tam || row < 0 || row >= acc::engine_radial::kRowCount) return nullptr;
    return reinterpret_cast<unsigned char*>(tam) +
           kTamTargetActionsOffset + row * kTargetActionStride;
}

void* RowActionButtonAddr(void* tam, int row) {
    void* rowBase = RowActionAddr(tam, row);
    if (!rowBase) return nullptr;
    return reinterpret_cast<unsigned char*>(rowBase) + kRowActionButtonOffset;
}

// Find the CSWGuiInterfaceAction* in action_lists[row].data that matches
// the engine's currently-selected action_id (field1[target_type*3 + row]).
// Returns data[0] when field1 holds -1 ("no selection, use default") or
// when no entry matches the stored id (engine fallback). Returns nullptr
// when the row is empty / the data pointer is null / a fault occurs.
void* FindSelectedActionDescriptor(void* tam, int row) {
    if (!tam || row < 0 || row >= acc::engine_radial::kRowCount) return nullptr;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(tam);
        size_t listOff = kTamActionListsOffset + row * kActionListStride;
        void* dataPtr = *reinterpret_cast<void**>(base + listOff);
        int32_t size  = *reinterpret_cast<int32_t*>(base + listOff + kActionListSizeOffset);
        if (!dataPtr || size <= 0) return nullptr;

        uint8_t targetType = *(base + kTamTargetTypeOffset);
        int32_t selectedId = kActionIdNone;
        if (targetType < 4) {
            selectedId = *reinterpret_cast<int32_t*>(
                base + kTamField1Offset +
                (static_cast<size_t>(targetType) * 3 + row) * sizeof(int32_t));
        }

        // -1 = "no selection, default to first"; otherwise scan for match
        // and fall through to data[0] when no entry has the stored id
        // (mirrors what the engine does in SelectNextAction's lookup loop).
        if (selectedId != kActionIdNone) {
            auto* entries = reinterpret_cast<unsigned char*>(dataPtr);
            for (int i = 0; i < size; ++i) {
                int32_t id = *reinterpret_cast<int32_t*>(
                    entries + i * kIfActionStride + kIfActionIdOffset);
                if (id == selectedId) {
                    return entries + i * kIfActionStride;
                }
            }
        }
        return dataPtr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

}  // namespace

bool ReadInt32(void* base, size_t offset, int32_t* out) {
    if (!base || !out) return false;
    __try {
        *out = *reinterpret_cast<int32_t*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Local copy of the gui_string read path used by ExtractAnnounceableText —
// pulled in here so the engine layer doesn't depend on menus.cpp's helpers.
// See engine_offsets.h for why we validate the vtable before dereffing
// gui_string.
bool ReadGuiStringLocal(void* control, size_t guiStringPtrOffset,
                        char* outBuf, size_t bufSize) {
    if (!control || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    __try {
        void* gs = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(control) + guiStringPtrOffset);
        if (!gs) return false;
        uintptr_t vt = *reinterpret_cast<uintptr_t*>(gs);
        if (vt != kVtableCAurGUIStringInternal) return false;
        const char* cstr = *reinterpret_cast<const char**>(
            reinterpret_cast<unsigned char*>(gs) + kAurGuiStringCStrOffset);
        if (!cstr) return false;
        size_t i = 0;
        for (; i + 1 < bufSize; ++i) {
            char c = cstr[i];
            outBuf[i] = c;
            if (c == '\0') return i > 0;
        }
        outBuf[i] = '\0';
        return i > 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
        return false;
    }
}

// Inline CExoString fallback. Same shape as engine_picker's ReadExoString
// but copy-pasted to avoid the cross-module include.
bool ReadCExoStringLocal(void* base, size_t offset,
                         char* outBuf, size_t bufSize) {
    if (!base || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    __try {
        auto* es = reinterpret_cast<CExoString*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        if (!es->c_string) return false;
        size_t i = 0;
        for (; i + 1 < bufSize; ++i) {
            char c = es->c_string[i];
            outBuf[i] = c;
            if (c == '\0') return i > 0;
        }
        outBuf[i] = '\0';
        return i > 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
        return false;
    }
}

void* ResolveTargetActionMenu() {
    void* mainIf = acc::engine::ResolveMainInterface();
    if (!mainIf) return nullptr;
    return reinterpret_cast<unsigned char*>(mainIf) +
           kMainInterfaceTargetActionMenuOffset;
}

int RowActionCount(void* tam, int row) {
    if (!tam || row < 0 || row >= kRowCount) return 0;
    int32_t size = 0;
    size_t offset = kTamActionListsOffset + row * kActionListStride +
                    kActionListSizeOffset;
    if (!ReadInt32(tam, offset, &size)) return 0;
    if (size < 0) return 0;
    return static_cast<int>(size);
}

uint32_t ReadSelectedRowActionId(void* tam, int row) {
    void* descriptor = FindSelectedActionDescriptor(tam, row);
    if (!descriptor) return 0;
    int32_t id = 0;
    __try {
        id = *reinterpret_cast<int32_t*>(
            reinterpret_cast<unsigned char*>(descriptor) + kIfActionIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return static_cast<uint32_t>(id);
}

bool ReadRowActionLabel(void* tam, int row, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    void* btn = RowActionButtonAddr(tam, row);
    if (!btn) return false;

    // gui_string is the rendered text — same priority order menus.cpp's
    // ExtractAnnounceableText uses for any other CSWGuiButton.
    if (ReadGuiStringLocal(btn, kButtonGuiStringPtrOffset, outBuf, bufSize)) {
        return true;
    }
    // Fall back to the inline CExoString. The engine sometimes leaves
    // gui_string null transiently between SelectNextAction calls.
    if (ReadCExoStringLocal(btn, kButtonTextOffset, outBuf, bufSize)) {
        return true;
    }
    // Source-of-truth fallback: read the label directly from
    // action_lists[row].data[selected]. This is the descriptor the engine
    // *will* render into action_button on its next paint pass — at populate
    // time the rendered button text is empty (verified in
    // patch-20260505-101621.log: action_lists[1].data[0] peek showed
    // [Security] while target_actions[1].action_button was []). Without
    // this fallback the user hears "Action 1/1" with no label on the very
    // first arm of a freshly-populated row.
    void* descriptor = FindSelectedActionDescriptor(tam, row);
    if (descriptor &&
        ReadCExoStringLocal(descriptor, kIfActionLabelOffset,
                            outBuf, bufSize)) {
        return true;
    }
    return false;
}

bool ReadTargetName(void* tam, char* outBuf, size_t bufSize) {
    if (!tam || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    void* nameLabel = reinterpret_cast<unsigned char*>(tam) + kTamNameLabelOffset;
    if (ReadGuiStringLocal(nameLabel, kLabelGuiStringPtrOffset, outBuf, bufSize)) {
        return true;
    }
    if (ReadCExoStringLocal(nameLabel, kLabelTextOffset, outBuf, bufSize)) {
        return true;
    }
    return false;
}

bool DispatchRowAction(void* tam, int row) {
    if (!tam || row < 0 || row >= kRowCount) return false;
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
    if (!tam || row < 0 || row >= kRowCount || index < 0) return false;
    __try {
        auto* base   = reinterpret_cast<unsigned char*>(tam);
        size_t listOff = kTamActionListsOffset + row * kActionListStride;
        void* dataPtr = *reinterpret_cast<void**>(base + listOff);
        int32_t size  = *reinterpret_cast<int32_t*>(
            base + listOff + kActionListSizeOffset);
        if (!dataPtr || index >= size) return false;

        auto* entries = reinterpret_cast<unsigned char*>(dataPtr);
        int32_t actionId = *reinterpret_cast<int32_t*>(
            entries + index * kIfActionStride + kIfActionIdOffset);
        if (actionId == 0) return false;

        uint8_t targetType = *(base + kTamTargetTypeOffset);
        if (targetType >= 4) return false;

        *reinterpret_cast<int32_t*>(
            base + kTamField1Offset +
            (static_cast<size_t>(targetType) * 3 + row) * sizeof(int32_t)) =
                actionId;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Radial", "SelectActionInRow SEH-FAULT row=%d idx=%d",
                      row, index);
        return false;
    }
}

uint32_t ReadRowActionIdAtIndex(void* tam, int row, int index) {
    if (!tam || row < 0 || row >= kRowCount || index < 0) return 0;
    __try {
        auto* base   = reinterpret_cast<unsigned char*>(tam);
        size_t listOff = kTamActionListsOffset + row * kActionListStride;
        void* dataPtr = *reinterpret_cast<void**>(base + listOff);
        int32_t size  = *reinterpret_cast<int32_t*>(
            base + listOff + kActionListSizeOffset);
        if (!dataPtr || index >= size) return 0;
        return static_cast<uint32_t>(*reinterpret_cast<int32_t*>(
            reinterpret_cast<unsigned char*>(dataPtr) +
            index * kIfActionStride + kIfActionIdOffset));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

int FindRowIndexByActionId(void* tam, int row, uint32_t actionId) {
    if (!tam || row < 0 || row >= kRowCount || actionId == 0) return -1;
    __try {
        auto* base   = reinterpret_cast<unsigned char*>(tam);
        size_t listOff = kTamActionListsOffset + row * kActionListStride;
        void* dataPtr = *reinterpret_cast<void**>(base + listOff);
        int32_t size  = *reinterpret_cast<int32_t*>(
            base + listOff + kActionListSizeOffset);
        if (!dataPtr) return -1;
        auto* entries = reinterpret_cast<unsigned char*>(dataPtr);
        for (int i = 0; i < size; ++i) {
            uint32_t id = static_cast<uint32_t>(*reinterpret_cast<int32_t*>(
                entries + i * kIfActionStride + kIfActionIdOffset));
            if (id == actionId) return i;
        }
        return -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int RetargetRowActions(void* tam, int row, uint32_t targetClientHandle) {
    if (!tam || row < 0 || row >= kRowCount) return -1;
    __try {
        auto* base   = reinterpret_cast<unsigned char*>(tam);
        size_t listOff = kTamActionListsOffset + row * kActionListStride;
        void* dataPtr = *reinterpret_cast<void**>(base + listOff);
        int32_t size  = *reinterpret_cast<int32_t*>(
            base + listOff + kActionListSizeOffset);
        if (!dataPtr || size <= 0) return -1;

        auto* entries = reinterpret_cast<unsigned char*>(dataPtr);
        uint32_t prev = *reinterpret_cast<uint32_t*>(
            entries + kIfActionTargetOffset);
        for (int i = 0; i < size; ++i) {
            *reinterpret_cast<uint32_t*>(
                entries + i * kIfActionStride + kIfActionTargetOffset) =
                    targetClientHandle;
        }
        if (prev != targetClientHandle) {
            acclog::Write("Radial",
                "retarget row=%d stale creature_id 0x%08x -> 0x%08x "
                "(%d entries)", row, prev, targetClientHandle, size);
        }
        return size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Radial", "RetargetRowActions SEH-FAULT row=%d", row);
        return -1;
    }
}

// Call vtable->AsSWCxxx(gameObject) at the given vtable byte-offset.
// Returns nullptr on null input or any fault.
void* CallVtableAsClass(void* gameObject, size_t vtableOffset) {
    if (!gameObject) return nullptr;
    __try {
        unsigned char** vtablePtr =
            *reinterpret_cast<unsigned char***>(gameObject);
        if (!vtablePtr) return nullptr;
        unsigned char* slot =
            *reinterpret_cast<unsigned char**>(
                reinterpret_cast<unsigned char*>(vtablePtr) + vtableOffset);
        if (!slot) return nullptr;
        auto fn = reinterpret_cast<PFN_AsClass>(slot);
        return fn(gameObject);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool IsCreatureClientTarget(uint32_t handle) {
    if (handle == 0u || handle == 0xFFFFFFFFu || handle == 0x7F000000u) {
        return false;
    }
    uint32_t client = handle | 0x80000000u;  // engine convention: client high bit
    void* exoApp = acc::engine::GetClientApp();
    if (!exoApp) return false;
    void* gameObject = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetGameObject>(kAddrCClientGetGameObject);
        gameObject = fn(exoApp, client);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!gameObject) return false;
    return CallVtableAsClass(gameObject, kVtableAsSWCCreatureOffset) != nullptr;
}

}  // namespace acc::engine_radial
