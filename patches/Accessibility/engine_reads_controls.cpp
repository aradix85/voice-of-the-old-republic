// What a GUI control is and its state: class predicates, id, flags, list box
// row count, toggle state.

#include "engine_reads.h"

#include <windows.h>
#include <cstdio>

namespace acc::engine {

int ReadControlId(void* control) {
    if (!control) return -1;
    __try {
        return *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(control) + kControlIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool IsToggle(void* control) {
    return CallDowncast(control, kVtableAsButtonToggle) != nullptr;
}

// Vtable-identity predicates run from per-tick menu monitors. A null-check
// alone isn't enough: panel-teardown windows (e.g. modal close → area load)
// can leave a freed-but-non-null control pointer in cached monitor state
// for one extra Dispatch() tick. Crash analysed 2026-05-11 (dump
// swkotor.exe.14028.dmp): IsSlider faulted at the vtable read on a freed
// PartySelection OK button right after `SubScreen.Status new_status=4`.
// SEH-guard the dereference so a stale pointer returns false (same as
// a real type mismatch) instead of access-violation-ing the process.
// CallDowncast above uses the same pattern for the IsToggle path.
bool IsSlider(void* control) {
    if (!control) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(control);
        return reinterpret_cast<uintptr_t>(vt) == kVtableSlider;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

uint32_t ReadControlBitFlags(void* control) {
    if (!control) return 0;
    __try {
        return *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(control) + kControlBitFlagsOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

bool IsControlVisible(void* control) {
    return (ReadControlBitFlags(control) & kControlVisibleBit) != 0;
}

int ReadListBoxRowCount(void* listBox) {
    if (!listBox) return -1;
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(listBox) + kListBoxControlsOffset);
        int size = list->size;
        if (size < 0 || size > 4096) return -1;
        return size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool IsListBox(void* control) {
    if (!control) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(control);
        return reinterpret_cast<uintptr_t>(vt) == kVtableListBox;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsEditbox(void* control) {
    if (!control) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(control);
        uintptr_t v = reinterpret_cast<uintptr_t>(vt);
        // CSWGuiSaveGameEditBox (save-name popup) is a subclass with its own
        // vtable but the same struct layout — accept it so its focus-enter
        // announce + field reads go through the same path as the plain editbox.
        return v == kVtableEditbox || v == kVtableSaveGameEditbox;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadToggleState(void* toggle) {
    return (ReadU32(toggle, kButtonToggleStateOffset) & 1u) != 0;
}

void DumpControlVtable(void* control, char* out, size_t outSize) {
    void** vtable = *reinterpret_cast<void***>(control);
    if (!vtable) {
        snprintf(out, outSize, "vtable=NULL");
        return;
    }
    snprintf(out, outSize,
             "vtable=%p [0]=%p [4]=%p [20]=%p [22]=%p",
             vtable, vtable[0], vtable[4], vtable[20], vtable[22]);
}

}  // namespace acc::engine
