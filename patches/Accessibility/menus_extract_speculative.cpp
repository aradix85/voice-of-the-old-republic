// The speculative text read for controls whose class overrides
// AsLabel/AsButton: the in-game menu icons and the chargen decorations.

#include "menus_extract_internal.h"
#include "engine_area.h"        // GetObjectDisplayNameByHandle
#include "engine_game.h"        // IsKotor2 — K1-only portraits.2da row table
#include "engine_panels.h"
#include "engine_player.h"      // PartyTableIsNPCAvailable / *Selectable /
#include "log.h"
#include "strings.h"
#include "engine_rebase.h"
#include "engine_offsets_select.h"

namespace acc::menus::extract {

// 8. Speculative text read for known label/button vtable overrides.
//    Some classes override AsLabel/AsButton in their vtable so that
//    CallDowncast returns null even though the class IS label-like or
//    button-like at the field-offset level. The InGameMenu icons are
//    the canonical case: 8 sibling labels at vtable=0x0073E8E8 and
//    8 image-only buttons at vtable=0x0073E658, and our standard
//    extraction returns nullptr for all of them (panel-walk shows
//    src=none).
//
//    For each entry in kKnownVtableOverrides we try a direct read at
//    the standard label/button text offsets, guarded by SEH so that
//    reading at an offset that's NOT a CExoString doesn't crash the
//    game. If the structure is different, we'll get a SEH exception
//    and silently fall through to the placeholder path.
//
//    Allowlist gating keeps speculative reads off random unknown
//    vtables — we only fire on classes we've observed needing this.
const char* TrySpeculativeVtableRead(void* control, char* outBuf,
                                     size_t bufSize) {
    const char* source = nullptr;
    struct VtableOverrideInfo {
        uintptr_t vtable;
        bool      tryLabel;
        bool      tryButton;
        const char* tag;
    };
    static const VtableOverrideInfo k_knownOverrides[] = {
        // Sibling labels of in-game-menu icons (Equipment, Inventory,
        // Character, ...) — observed children [0..7] of CSWGuiInGameMenu.
        // Also chargen wizard step-number decorations.
        { 0x0073E8E8, true,  false, "label-spec" },
        // Image-only buttons (in-game-menu icons children [8..15],
        // chargen class icons, portrait-picker arrows).
        { 0x0073E658, false, true,  "button-spec" },
        // PartySelection portraits (vtable 0x00756BB8) are handled
        // in section 7b above — they store an NPC roster index, not
        // inline text, so the speculative offset reader never
        // resolves anything useful for them.
    };
    // Guarded like TryPartyPortrait's head: a walked control slot can hold a
    // non-null garbage pointer, and the vtable read is the first deref.
    void** vt = nullptr;
    __try {
        vt = *reinterpret_cast<void***>(control);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    uintptr_t vta = reinterpret_cast<uintptr_t>(vt);
    for (const auto& ov : k_knownOverrides) {
        // Table stays reference-build and compile-time: giving it a runtime
        // initialiser would make this function need object unwinding, which
        // its __try blocks below forbid (C2712). Map at the comparison.
        // R() returns 0 for a vtable with no KOTOR 2 mapping — without the
        // null guard, every garbage control whose first dword reads 0
        // "matches" both overrides and eats two SEH faults per visit (the
        // vtable=0x0 storm in userlogs/kennycrash0782408).
        uintptr_t ovMapped = acc::addr::R(ov.vtable);
        if (!ovMapped || ovMapped != vta) continue;
        char text[256];
        bool got = false;
        if (ov.tryLabel) {
            __try {
                // Path A: inline CExoString at standard label offset.
                if (ReadCExoString(control, kLabelTextOffset,
                                   text, sizeof(text))) {
                    got = true;
                }
                // Path B: strref at standard label offset → TLK.
                if (!got) {
                    uint32_t strref = ReadU32(control, kLabelStrRefOffset);
                    got = LookupTlk(strref, text, sizeof(text));
                }
                // Path C: text_object indirection. CSWGuiLabel.text.
                // text_params.text_object is a CSWGuiText* at +0x138; if
                // non-null, read its text_params.text (CExoString @+0x18)
                // or text_params.str_ref (@+0x20). Many labelhilights
                // route their rendered text through this pointer rather
                // than the inline CExoString.
                if (!got) {
                    void* textObj = *reinterpret_cast<void**>(
                        reinterpret_cast<unsigned char*>(control)
                        + kLabelTextObjectOffset);
                    if (textObj) {
                        if (ReadCExoString(textObj, kTextObjectTextOffset,
                                           text, sizeof(text))) {
                            got = true;
                        } else {
                            uint32_t strref = ReadU32(textObj,
                                                      kTextObjectStrRefOffset);
                            got = LookupTlk(strref, text, sizeof(text));
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                acclog::Write("Menus.SpecRead", "label SEH for vtable=0x%x "
                              "control=%p", (unsigned)vta, control);
                got = false;
            }
        }
        if (!got && ov.tryButton) {
            __try {
                // Path A: inline CExoString at standard button offset.
                if (ReadCExoString(control, kButtonTextOffset,
                                   text, sizeof(text))) {
                    got = true;
                }
                // Path B: strref at standard button offset → TLK.
                if (!got) {
                    uint32_t strref = ReadU32(control, kButtonStrRefOffset);
                    got = LookupTlk(strref, text, sizeof(text));
                }
                // Path C: text_object indirection at +0x1BC (button-side).
                if (!got) {
                    void* textObj = *reinterpret_cast<void**>(
                        reinterpret_cast<unsigned char*>(control)
                        + kButtonTextObjectOffset);
                    if (textObj) {
                        if (ReadCExoString(textObj, kTextObjectTextOffset,
                                           text, sizeof(text))) {
                            got = true;
                        } else {
                            uint32_t strref = ReadU32(textObj,
                                                      kTextObjectStrRefOffset);
                            got = LookupTlk(strref, text, sizeof(text));
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                acclog::Write("Menus.SpecRead", "button SEH for vtable=0x%x "
                              "control=%p", (unsigned)vta, control);
                got = false;
            }
        }
        if (got) {
            if (EmitText(outBuf, bufSize, text)) {
                source = ov.tag;
                // Trace: chain rebind / step / fingerprint all visit the
                // same control multiple times per arrow press; collapse
                // identical hit/empty/miss runs while preserving the
                // suppressed count.
                acclog::Trace("Menus.SpecRead",
                              "hit vtable=0x%x control=%p text=\"%s\"",
                              (unsigned)vta, control, outBuf);
                break;
            }
            acclog::Trace("Menus.SpecRead",
                          "empty vtable=0x%x control=%p "
                          "(read returned but text was empty)",
                          (unsigned)vta, control);
        } else {
            acclog::Trace("Menus.SpecRead",
                          "miss vtable=0x%x control=%p tag=%s",
                          (unsigned)vta, control, ov.tag);
        }
    }
    return source;
}

}  // namespace acc::menus::extract
