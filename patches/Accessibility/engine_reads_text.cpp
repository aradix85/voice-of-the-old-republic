// Text of GUI controls: the rendered gui_string, tooltips, and label, button
// and edit box text.

#include "engine_reads.h"

#include <windows.h>
#include <cstring>

#include "log.h"

namespace acc::engine {

namespace {

// True iff buf looks like real localised text rather than uninitialised
// memory. The action-bar / target-action / radial column buttons leave
// their CSWGuiControl.tooltip_string slot at +0x28 uninitialised
// (engine renders these buttons via a separate dynamic-text path, so the
// .gui-time CExoString never gets written). What we observed in practice
// is a non-null literal pointer + a small length (3 bytes) yielding
// CP1252 control-range bytes 0x80..0x9F — code points the engine never
// emits in any localised UI string. Rejecting those lets the caller fall
// back to the proper "No description available" cue.
bool LooksLikeReadableText(const char* buf, size_t len) {
    if (!buf || len == 0) return false;
    size_t printable = 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = static_cast<unsigned char>(buf[i]);
        // ASCII printable + extended Latin range (covers German umlauts
        // 0xE4/0xF6/0xFC/0xDF, accented French/Spanish/Italian, etc.).
        // Excludes 0x00..0x1F (controls), 0x7F (DEL), and 0x80..0x9F
        // (CP1252 control block — never used in localised UI strings).
        if ((c >= 0x20 && c < 0x7F) || c >= 0xA0) {
            ++printable;
        }
    }
    // Require at least one printable byte AND a majority of printable
    // bytes. Short all-garbage strings (the 3-byte case we hit) get
    // rejected; legitimate short tooltips like "OK" keep working.
    return printable > 0 && printable * 2 >= len;
}

}  // namespace

bool ReadControlNameFields(void* control, const char*& outTip,
                           uint32_t& outTipLen, int& outId) {
    // Always initialise the outs: callers (OnHandleFocusChange) log them
    // unconditionally, so an early return must not leave them garbage.
    outTip    = nullptr;
    outTipLen = 0;
    outId     = 0;
    if (!control) return false;
    // SEH-guarded like CallDowncast below. This is called straight from the
    // OnHandleFocusChange detour, which fires during engine control
    // teardown - exactly the window where `control` is stale rather than
    // null, which a null check cannot catch.
    __try {
        auto* base = reinterpret_cast<unsigned char*>(control);
        outTip    = *reinterpret_cast<const char**>(base + kControlTooltipStringOffset);
        outTipLen = *reinterpret_cast<uint32_t*>(base + kControlTooltipStringOffset + 4);
        outId     = *reinterpret_cast<int*>(base + kControlIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outTip    = nullptr;
        outTipLen = 0;
        outId     = 0;
        return false;
    }
    return outTip && outTipLen > 0;
}

bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen) {
    out[0] = '\0';
    outLen = 0;
    void* cstr = nullptr;
    if (!TryReadPtr(editbox, kEditboxStringCStrOffset, &cstr)) return false;
    if (!cstr) return true;
    __try {
        const size_t n = strnlen(static_cast<const char*>(cstr), cap - 1);
        memcpy(out, cstr, n);
        out[n] = '\0';
        outLen = static_cast<uint32_t>(n);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out[0] = '\0';
        outLen = 0;
        return false;
    }
    return true;
}

bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize) {
    if (!control || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';

    // Bound the parent-walk so a malformed/cyclic parent chain can't spin
    // us forever. The deepest in-game panel hierarchy we've observed is
    // ~6 levels (panel → row → embedded button → text); 8 is generous.
    void*    cur   = control;
    void*    last  = nullptr;
    int      hops  = 0;
    while (cur && cur != last && hops < 8) {
        last = cur;
        ++hops;

        uint32_t strref     = 0;
        const char* literal = nullptr;
        uint32_t literalLen = 0;
        void*    parent     = nullptr;

        __try {
            auto* base = reinterpret_cast<unsigned char*>(cur);
            strref  = *reinterpret_cast<uint32_t*>(base + kControlTooltipStrRefOffset);
            literal = *reinterpret_cast<const char**>(base + kControlTooltipStringOffset);
            literalLen = *reinterpret_cast<uint32_t*>(
                base + kControlTooltipStringOffset + 4);
            parent  = *reinterpret_cast<void**>(base + kControlParentOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        // 1. Strref takes priority (engine decompile: when field4_0x24
        //    is non-zero AND a strref lookup succeeds, that wins over
        //    the literal).
        if (strref != 0 && strref != 0xFFFFFFFF) {
            if (LookupTlk(strref, outBuf, bufSize) && outBuf[0]) {
                return true;
            }
        }

        // 2. Literal tooltip_string. Validate it looks like real text —
        //    action-bar / target-action / radial column buttons leave
        //    this slot uninitialised and the engine never wipes it on
        //    .gui load, so a stale non-null pointer + small length can
        //    return CP1252 control-range garbage. Drop garbage and keep
        //    bubbling so the caller's "no tooltip" fallback fires.
        if (literal && literalLen > 0 && literalLen < bufSize) {
            __try {
                memcpy(outBuf, literal, literalLen);
                outBuf[literalLen] = '\0';
                if (LooksLikeReadableText(outBuf, literalLen)) {
                    return true;
                }
                outBuf[0] = '\0';
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                outBuf[0] = '\0';
                // Fall through to parent walk — corrupt literal pointer.
            }
        }

        // 3. Bubble up to parent and retry (engine recurses via
        //    parent_control->vtable->DisplayToolTip).
        cur = parent;
    }

    return false;
}

// True for the byte-level glyph markers KOTOR 2 embeds in a rendered
// gui_string. The Aspyr build prefixes an actionable button's caption with a
// controller-icon code — 0x11 followed by a space — so the close button on
// every sub-screen renders as "\x11 Close" while dialog.tlk 1582 says
// "Close". Not whitespace, so no amount of trimming reaches it; it has to
// come off as a distinct class of byte.
//
// Whitespace controls are deliberately NOT in the set. '\n' in particular
// carries meaning downstream — the Options settings blob is identified by
// interior newlines (menus_setactive.cpp) — so only the non-whitespace C0
// codes are treated as markers.
bool IsGlyphMarkerByte(unsigned char ch) {
    return ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r';
}

bool ReadGuiString(void* control, size_t guiStringPtrOffset,
                   char* outBuf, size_t bufSize) {
    if (!control || bufSize < 2) return false;
    bool got = false;
    __try {
        void* guiString = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(control) + guiStringPtrOffset);
        if (!guiString) return false;
        uintptr_t gsVtable = *reinterpret_cast<uintptr_t*>(guiString);
        if (gsVtable != kVtableCAurGUIStringInternal) return false;
        char* str = *reinterpret_cast<char**>(
            reinterpret_cast<unsigned char*>(guiString) + kAurGuiStringCStrOffset);
        if (!str) return false;
        size_t len = 0;
        while (len < bufSize - 1 && str[len] != '\0') ++len;
        if (len == 0) return false;
        memcpy(outBuf, str, len);
        outBuf[len] = '\0';
        got = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads", "ReadGuiString SEH for control=%p offset=0x%x",
                      control, (unsigned)guiStringPtrOffset);
        got = false;
    }
    if (!got) return false;

    // Strip the glyph markers off both ends. Done here, at the single funnel
    // every gui_string read goes through, rather than at each comparison site
    // — the marker is not text, and letting it through means both a caption
    // that no strref compare can match AND a stray control byte handed to the
    // screen reader on every K2 button.
    size_t start = 0;
    while (outBuf[start] &&
           IsGlyphMarkerByte(static_cast<unsigned char>(outBuf[start]))) {
        ++start;
    }
    size_t end = strlen(outBuf + start);
    while (end > 0 &&
           IsGlyphMarkerByte(static_cast<unsigned char>(outBuf[start + end - 1]))) {
        --end;
    }
    if (start != 0) memmove(outBuf, outBuf + start, end);
    outBuf[end] = '\0';

    // Marker-only strings (a pure icon button) count as no text, so the
    // caller falls through to its CExoString / strref paths instead of
    // accepting an empty rendered string as the answer.
    return outBuf[0] != '\0';
}

bool ExtractTextOrStrRefIndirect(void* control,
                                 size_t cexoOffset, size_t strRefOffset,
                                 size_t textObjectOffset,
                                 char* outBuf, size_t bufSize) {
    // gui_string offset for label and button differ by inline CSWGuiText
    // start offset; derive from cexoOffset to avoid threading another
    // parameter through every call site (gui_string ptr is at
    // text.+0x14 = (cexo - 0x18) + 0x14 = cexo - 4).
    size_t guiStringPtrOffset = cexoOffset - 4;
    if (ReadGuiString(control, guiStringPtrOffset, outBuf, bufSize)) {
        return true;
    }
    if (ExtractTextOrStrRef(control, cexoOffset, strRefOffset, outBuf, bufSize)) {
        return true;
    }
    bool got = false;
    __try {
        void* textObj = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(control) + textObjectOffset);
        if (textObj) {
            got = ExtractTextOrStrRef(textObj,
                                      kTextObjectTextOffset,
                                      kTextObjectStrRefOffset,
                                      outBuf, bufSize);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads", "text_object indirection SEH for control=%p "
                      "(textObjectOffset=0x%x)", control, (unsigned)textObjectOffset);
        got = false;
    }
    return got;
}

bool ReadLabelText(void* label, char* outBuf, size_t bufSize) {
    if (!label || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    __try {
        if (ReadGuiString(label, kLabelGuiStringPtrOffset,
                          outBuf, bufSize) && outBuf[0] != '\0') {
            return true;
        }
        if (ExtractTextOrStrRefIndirect(
                label, kLabelTextOffset, kLabelStrRefOffset,
                kLabelTextObjectOffset, outBuf, bufSize) &&
            outBuf[0] != '\0') {
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
    }
    return false;
}

bool ReadButtonText(void* button, char* outBuf, size_t bufSize) {
    if (!button || !outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    __try {
        if (ReadGuiString(button, kButtonGuiStringPtrOffset,
                          outBuf, bufSize) && outBuf[0] != '\0') {
            return true;
        }
        if (ExtractTextOrStrRefIndirect(
                button, kButtonTextOffset, kButtonStrRefOffset,
                kButtonTextObjectOffset, outBuf, bufSize) &&
            outBuf[0] != '\0') {
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outBuf[0] = '\0';
    }
    return false;
}

}  // namespace acc::engine
