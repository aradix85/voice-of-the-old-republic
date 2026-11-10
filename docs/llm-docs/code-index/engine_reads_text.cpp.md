# engine_reads_text.cpp (201 lines)

Source: `patches/Accessibility/engine_reads_text.cpp`

Text of GUI controls: the rendered gui_string, tooltips, and label, button and edit box text.

## Declarations (in source order)

- L20 `bool LooksLikeReadableText(const char* buf, size_t len)`: True when buf looks like text rather than uninitialised memory: mostly printable bytes (ASCII or 0xA0 and up; the CP1252 control block 0x80..0x9F never occurs in UI strings).
- L34 `bool IsGlyphMarkerByte(unsigned char ch)`: True for the glyph markers KOTOR 2 embeds in a rendered gui_string: the Aspyr build prefixes an actionable button's caption with a controller-icon code, so Close renders as "\x11 Close".
- L40 `bool ReadControlNameFields(void* control, const char*& outTip, uint32_t& outTipLen, int& outId)`
- L61 `bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen)`
- L80 `bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize)`
- L108 `bool ReadGuiString(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`
- L155 `bool ExtractTextOrStrRefIndirect(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`
- L176 `bool ReadTextOrEmpty(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`: Non-empty text through every path, "" on a miss.
- L190 `bool ReadLabelText(void* label, char* outBuf, size_t bufSize)`
- L195 `bool ReadButtonText(void* button, char* outBuf, size_t bufSize)`
