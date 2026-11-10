// What a GUI control is and its state: class predicates, id, flags, list box
// row count, toggle state. Every read is guarded: a non-null control can be
// freed (a panel teardown leaves stale pointers in monitor state for a
// tick), and a stale one must read as "not this class", not crash.

#include "engine_reads.h"

#include <cstdio>

namespace acc::engine {

namespace {

// The control's vtable address, 0 on fault.
uintptr_t VtableOf(void* control) {
    void* vt = nullptr;
    TryReadPtr(control, 0, &vt);
    return reinterpret_cast<uintptr_t>(vt);
}

}  // namespace

int ReadControlId(void* control) {
    uint32_t id = 0;
    return TryReadU32(control, kControlIdOffset, &id) ? static_cast<int>(id) : -1;
}

bool IsToggle(void* control) {
    return CallDowncast(control, kVtableAsButtonToggle) != nullptr;
}

bool IsSlider(void* control) {
    return control && VtableOf(control) == kVtableSlider;
}

bool IsListBox(void* control) {
    return control && VtableOf(control) == kVtableListBox;
}

// CSWGuiSaveGameEditBox (the save-name popup) is a subclass with its own
// vtable and the same layout.
bool IsEditbox(void* control) {
    if (!control) return false;
    const uintptr_t vt = VtableOf(control);
    return vt == kVtableEditbox || vt == kVtableSaveGameEditbox;
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

bool ReadToggleState(void* toggle) {
    uint32_t state = 0;
    TryReadU32(toggle, kButtonToggleStateOffset, &state);
    return (state & 1u) != 0;
}

void DumpControlVtable(void* control, char* out, size_t outSize) {
    void* vtable = reinterpret_cast<void*>(VtableOf(control));
    void* slot[4] = {};
    const size_t kSlots[4] = {0, 4, 20, 22};
    for (int i = 0; i < 4; ++i) {
        TryReadPtr(vtable, kSlots[i] * sizeof(void*), &slot[i]);
    }
    snprintf(out, outSize, "vtable=%p [0]=%p [4]=%p [20]=%p [22]=%p",
             vtable, slot[0], slot[1], slot[2], slot[3]);
}

}  // namespace acc::engine
