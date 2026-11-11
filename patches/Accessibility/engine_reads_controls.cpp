// What a GUI control is and its state: class predicates, id, flags, list box
// row count, toggle state. Every read is guarded: a non-null control can be
// freed (a panel teardown leaves stale pointers in monitor state for a
// tick), and a stale one must read as "not this class", not crash.

#include "engine_reads.h"

#include <windows.h>
#include <cstddef>
#include <cstdio>

namespace acc::engine {

int ReadControlId(void* control) {
    uint32_t id = 0;
    return TryReadU32(control, kControlIdOffset, &id) ? static_cast<int>(id) : -1;
}

bool IsToggle(void* control) {
    return CallDowncast(control, kVtableAsButtonToggle) != nullptr;
}

bool IsSlider(void* control) {
    return HasVtable(control, kVtableSlider);
}

bool IsListBox(void* control) {
    return HasVtable(control, kVtableListBox);
}

// CSWGuiSaveGameEditBox (the save-name popup) is a subclass with its own
// vtable and the same layout.
bool IsEditbox(void* control) {
    return HasVtable(control, kVtableEditbox) ||
           HasVtable(control, kVtableSaveGameEditbox);
}

uint32_t ReadControlBitFlags(void* control) {
    uint32_t flags = 0;
    TryReadU32(control, kControlBitFlagsOffset, &flags);
    return flags;
}

bool IsControlVisible(void* control) {
    return (ReadControlBitFlags(control) & kControlVisibleBit) != 0;
}

int ReadListBoxRowCount(void* listBox) {
    uint32_t size = 0;
    if (!TryReadU32(listBox, kListBoxControlsOffset + offsetof(CExoArrayList, size),
                    &size) ||
        size > 4096) {
        return -1;
    }
    return static_cast<int>(size);
}

namespace {

// The controls array's data pointer; false when unreadable or null.
bool ReadListBoxRowArray(void* listBox, void** outData) {
    return TryReadPtr(listBox, kListBoxControlsOffset + offsetof(CExoArrayList, data),
                      outData) &&
           *outData;
}

}  // namespace

void* ReadListBoxRow(void* listBox, int index) {
    void* data = nullptr;
    void* row = nullptr;
    if (index < 0 || index >= ReadListBoxRowCount(listBox) ||
        !ReadListBoxRowArray(listBox, &data) ||
        !TryReadPtr(data, static_cast<size_t>(index) * sizeof(void*), &row)) {
        return nullptr;
    }
    return row;
}

int ReadListBoxSelection(void* listBox) {
    uint16_t selection = 0;
    if (!TryReadU16(listBox, kListBoxSelectionIndexOffset, &selection)) return -1;
    return static_cast<int16_t>(selection);
}

void* ReadListBoxSelectedRow(void* listBox) {
    return ReadListBoxRow(listBox, ReadListBoxSelection(listBox));
}

int FindListBoxRow(void* listBox, void* row) {
    const int count = ReadListBoxRowCount(listBox);
    void* data = nullptr;
    if (!row || count <= 0 || !ReadListBoxRowArray(listBox, &data)) return -1;
    __try {
        void* const* rows = static_cast<void* const*>(data);
        for (int i = 0; i < count; ++i) {
            if (rows[i] == row) return i;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // An array that ends early holds no such row.
    }
    return -1;
}

bool ReadToggleState(void* toggle) {
    uint32_t state = 0;
    TryReadU32(toggle, kButtonToggleStateOffset, &state);
    return (state & 1u) != 0;
}

void DumpControlVtable(void* control, char* out, size_t outSize) {
    void* vtable = reinterpret_cast<void*>(ReadVtable(control));
    void* slot[4] = {};
    const size_t kSlots[4] = {0, 4, 20, 22};
    for (int i = 0; i < 4; ++i) {
        TryReadPtr(vtable, kSlots[i] * sizeof(void*), &slot[i]);
    }
    snprintf(out, outSize, "vtable=%p [0]=%p [4]=%p [20]=%p [22]=%p",
             vtable, slot[0], slot[1], slot[2], slot[3]);
}

}  // namespace acc::engine
