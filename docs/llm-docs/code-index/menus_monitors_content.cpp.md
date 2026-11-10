# menus_monitors_content.cpp (343 lines)

Source: `patches/Accessibility/menus_monitors_content.cpp`

The content monitor: a text fingerprint of each monitored panel, and speaking the segments that are new since the last tick.

## Declarations (in source order)

- L29 `bool IsContentMonitored(PanelKind k)`
- L105 `bool IsContentSpoken(PanelKind k)`: Of the content-monitored kinds, which may SPEAK their diff.
- L115 `void BuildContentFingerprint(void* panel, char* out, size_t outSize)`
- L171 `bool FingerprintContainsSegment(const char* hay, size_t hayLen, const char* seg, size_t segLen)`
- L191 `void SpeakNewSegments(const char* prev, const char* curr)`
- L226 `char* GetContentSnapshot(void* panel)`
- L245 `void MonitorPanelContents()`
