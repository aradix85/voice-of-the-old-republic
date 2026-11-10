// Text of GUI controls: the rendered gui_string, tooltips, and label, button
// and edit box text.

#include "engine_reads.h"

#include <windows.h>
#include <cstring>

#include "log.h"

namespace acc::engine {

namespace {

// True when buf looks like text rather than uninitialised memory: mostly
// printable bytes (ASCII or 0xA0 and up; the CP1252 control block
// 0x80..0x9F never occurs in UI strings). The action bar, target action and
// radial buttons never write their tooltip_string, and what sits there is a
// non-null pointer with a small length of such control bytes.
bool LooksLikeReadableText(const char* buf, size_t len) {
    size_t printable = 0;
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(buf[i]);
        if ((c >= 0x20 && c < 0x7F) || c >= 0xA0) ++printable;
    }
    return printable > 0 && printable * 2 >= len;
}

// True for the glyph markers KOTOR 2 embeds in a rendered gui_string: the
// Aspyr build prefixes an actionable button's caption with a controller-icon
// code, so Close renders as "\x11 Close". Whitespace controls are not
// markers: '\n' carries meaning downstream (the Options settings blob is
// recognised by its newlines, menus_setactive.cpp).
bool IsGlyphMarkerByte(unsigned char ch) {
    return ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r';
}

}  // namespace

bool ReadControlNameFields(void* control, const char*& outTip,
                           uint32_t& outTipLen, int& outId) {
    // The caller logs the outs whatever the result.
    outTip    = nullptr;
    outTipLen = 0;
    outId     = 0;
    if (!control) return false;
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

    // The deepest panel hierarchy seen is about 6 levels; 8 hops also stop a
    // cyclic parent chain.
    void* cur = control;
    for (int hops = 0; cur && hops < 8; ++hops) {
        uint32_t strref = 0;
        void*    parent = nullptr;
        if (!TryReadU32(cur, kControlTooltipStrRefOffset, &strref) ||
            !TryReadPtr(cur, kControlParentOffset, &parent)) {
            return false;
        }
        // The strref wins over the literal when it resolves.
        if (LookupTlk(strref, outBuf, bufSize) && outBuf[0]) return true;
        // Garbage in the literal counts as no tooltip here; keep bubbling.
        if (ReadCExoString(cur, kControlTooltipStringOffset, outBuf, bufSize) &&
            LooksLikeReadableText(outBuf, strlen(outBuf))) {
            return true;
        }
        outBuf[0] = '\0';
        if (parent == cur) break;
        cur = parent;
    }
    return false;
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

    // Strip the glyph markers off both ends, here where every gui_string
    // read passes: a marker left in defeats strref compares and reaches the
    // screen reader as a stray control byte.
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

    // A marker-only string (an icon button) is no text: the caller goes on
    // to its CExoString and strref paths.
    return outBuf[0] != '\0';
}

bool ExtractTextOrStrRefIndirect(void* control,
                                 size_t cexoOffset, size_t strRefOffset,
                                 size_t textObjectOffset,
                                 char* outBuf, size_t bufSize) {
    // The gui_string pointer sits 4 bytes before the inline CExoString in
    // both CSWGuiText layouts (text +0x14 against +0x18), which saves a
    // parameter: kLabelGuiStringPtrOffset == kLabelTextOffset - 4, and the
    // same for buttons.
    if (ReadGuiString(control, cexoOffset - 4, outBuf, bufSize)) return true;
    if (ExtractTextOrStrRef(control, cexoOffset, strRefOffset, outBuf, bufSize)) {
        return true;
    }
    void* textObj = nullptr;
    return TryReadPtr(control, textObjectOffset, &textObj) && textObj &&
           ExtractTextOrStrRef(textObj, kTextObjectTextOffset,
                               kTextObjectStrRefOffset, outBuf, bufSize);
}

namespace {

// Non-empty text through every path, "" on a miss.
bool ReadTextOrEmpty(void* control, size_t cexoOffset, size_t strRefOffset,
                     size_t textObjectOffset, char* outBuf, size_t bufSize) {
    if (!control || !outBuf || bufSize == 0) return false;
    if (ExtractTextOrStrRefIndirect(control, cexoOffset, strRefOffset,
                                    textObjectOffset, outBuf, bufSize) &&
        outBuf[0] != '\0') {
        return true;
    }
    outBuf[0] = '\0';
    return false;
}

}  // namespace

bool ReadLabelText(void* label, char* outBuf, size_t bufSize) {
    return ReadTextOrEmpty(label, kLabelTextOffset, kLabelStrRefOffset,
                           kLabelTextObjectOffset, outBuf, bufSize);
}

bool ReadButtonText(void* button, char* outBuf, size_t bufSize) {
    return ReadTextOrEmpty(button, kButtonTextOffset, kButtonStrRefOffset,
                           kButtonTextObjectOffset, outBuf, bufSize);
}

}  // namespace acc::engine
