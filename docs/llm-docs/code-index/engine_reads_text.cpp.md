# engine_reads_text.cpp (296 lines)

Source: `patches/Accessibility/engine_reads_text.cpp`

Text of GUI controls: the rendered gui_string, tooltips, and label, button and edit box text.

## Declarations (in source order)

- L24 `bool LooksLikeReadableText(const char* buf, size_t len)`: True iff buf looks like real localised text rather than uninitialised memory.
- L45 `bool ReadControlNameFields(void* control, const char*& outTip, uint32_t& outTipLen, int& outId)`
- L71 `bool ReadEditboxText(void* editbox, char* out, size_t cap, uint32_t& outLen)`
- L90 `bool ReadControlTooltip(void* control, char* outBuf, size_t bufSize)`
- L168 `bool IsGlyphMarkerByte(unsigned char ch)`: True for the byte-level glyph markers KOTOR 2 embeds in a rendered gui_string.
- L172 `bool ReadGuiString(void* control, size_t guiStringPtrOffset, char* outBuf, size_t bufSize)`
- L222 `bool ExtractTextOrStrRefIndirect(void* control, size_t cexoOffset, size_t strRefOffset, size_t textObjectOffset, char* outBuf, size_t bufSize)`
- L255 `bool ReadLabelText(void* label, char* outBuf, size_t bufSize)`
- L275 `bool ReadButtonText(void* button, char* outBuf, size_t bufSize)`
