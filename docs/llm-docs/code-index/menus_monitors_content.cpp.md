# menus_monitors_content.cpp (321 lines)

Source: `patches/Accessibility/menus_monitors_content.cpp`

The content monitor: a text fingerprint of each monitored panel, and speaking the segments that are new since the last tick.

## Declarations (in source order)

- L31 `bool IsContentMonitored(PanelKind k)`
- L100 `bool IsContentSpoken(PanelKind k)`: Of the content-monitored kinds, which may SPEAK their diff.
- L110 `void BuildContentFingerprint(void* panel, char* out, size_t outSize)`
- L156 `bool FingerprintContainsSegment(const char* hay, size_t hayLen, const char* seg, size_t segLen)`
- L171 `void SpeakNewSegments(const char* prev, const char* curr)`
- L204 `char* GetContentSnapshot(void* panel)`
- L223 `void MonitorPanelContents()`
