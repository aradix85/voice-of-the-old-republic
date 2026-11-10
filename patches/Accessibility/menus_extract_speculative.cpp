// The speculative text read for controls whose class overrides
// AsLabel/AsButton, so the downcast returns null although the control is
// label- or button-shaped at the field level: the in-game menu icons (8
// labels and 8 image-only buttons), chargen class icons and decorations.
// Only allow-listed vtables are tried.

#include "menus_extract_internal.h"

#include "engine_rebase.h"  // R: the reference-build vtables
#include "log.h"

namespace acc::menus::extract {

namespace {

struct VtableOverride {
    uintptr_t   vtable;  // reference build, mapped at the comparison
    bool        label;   // label offsets, else button offsets
    const char* tag;
};

constexpr VtableOverride kOverrides[] = {
    // The menu icons' sibling labels, the chargen step numbers.
    {0x0073E8E8, true,  "label-spec"},
    // Image-only buttons: menu icons, chargen class icons, portrait arrows.
    // (Party portraits store a roster index, not text: TryPartyPortrait.)
    {0x0073E658, false, "button-spec"},
};

// Inline CExoString, then strref, then the text_object's own text or strref
// (labelhilights route their rendered text through it).
bool ReadAt(void* control, size_t textOff, size_t strrefOff, size_t textObjOff,
            char* text, size_t size) {
    if (ExtractTextOrStrRef(control, textOff, strrefOff, text, size)) return true;
    void* textObj = nullptr;
    return TryReadPtr(control, textObjOff, &textObj) && textObj &&
           ExtractTextOrStrRef(textObj, kTextObjectTextOffset, kTextObjectStrRefOffset,
                               text, size);
}

}  // namespace

const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize) {
    // A walked control slot can hold a non-null garbage pointer.
    void* vt = nullptr;
    if (!TryReadPtr(control, 0, &vt)) return nullptr;
    const uintptr_t vta = reinterpret_cast<uintptr_t>(vt);
    for (const VtableOverride& ov : kOverrides) {
        // R() is 0 for a vtable with no K2 mapping: without the check every
        // garbage control whose first dword reads 0 would match.
        const uintptr_t mapped = acc::addr::R(ov.vtable);
        if (!mapped || mapped != vta) continue;
        char text[256];
        const bool got =
            ov.label ? ReadAt(control, kLabelTextOffset, kLabelStrRefOffset,
                              kLabelTextObjectOffset, text, sizeof(text))
                     : ReadAt(control, kButtonTextOffset, kButtonStrRefOffset,
                              kButtonTextObjectOffset, text, sizeof(text));
        // Traces: rebind, step and fingerprint visit a control several times
        // per arrow press, and the dedup keeps the count.
        if (!got) {
            acclog::Trace("Menus.SpecRead", "miss vtable=0x%x control=%p tag=%s",
                          static_cast<unsigned>(vta), control, ov.tag);
            continue;
        }
        if (EmitText(outBuf, bufSize, text)) {
            acclog::Trace("Menus.SpecRead", "hit vtable=0x%x control=%p text=\"%s\"",
                          static_cast<unsigned>(vta), control, outBuf);
            return ov.tag;
        }
        acclog::Trace("Menus.SpecRead", "empty vtable=0x%x control=%p "
                      "(read returned but text was empty)", static_cast<unsigned>(vta), control);
    }
    return nullptr;
}

}  // namespace acc::menus::extract
