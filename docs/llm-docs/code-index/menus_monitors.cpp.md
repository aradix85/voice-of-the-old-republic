# menus_monitors.cpp (1247 lines)

Source: `patches/Accessibility/menus_monitors.cpp`

general per-tick monitors.

Post-Step-5 cleanup. See menus_monitors.h for the rationale and public surface. Function bodies are unchanged from the inline originals in menus.cpp; only namespacing and linkage changed.

## Declarations (in source order)

- L83 `bool acc::menus::detail::ParkCursorToCorner(const char* tag)`
- L108 `void AnnounceControl(void* control)`
- L199 `void MonitorFocusedControl()`
- L416 `uint32_t SpecStrref(const InGameSubScreenSpec* s)`: Per-game strref for a spec row.
- L421 `const InGameSubScreenSpec* FindSpec(PanelKind k)`
- L432 `bool IsSubScreenTracked(void* p)`
- L439 `void AnnounceNewSubScreens(void** panels, int count)`
- L543 `bool IsContentMonitored(PanelKind k)`
- L619 `bool IsContentSpoken(PanelKind k)`: Of the content-monitored kinds, which may SPEAK their diff.
- L629 `void BuildContentFingerprint(void* panel, char* out, size_t outSize)`
- L685 `bool FingerprintContainsSegment(const char* hay, size_t hayLen, const char* seg, size_t segLen)`
- L705 `void SpeakNewSegments(const char* prev, const char* curr)`
- L740 `char* GetContentSnapshot(void* panel)`
- L757 `void MonitorPanelContents()`
- L875 `unsigned ComputeReplySetSignature(int replyCount, int rowCount)`: FNV-1a over every active reply's text plus the count.
- L894 `bool ParkDialogCursorOffReplies(void* replyLb)`
- L904 `bool IsDialogPanelKind(PanelKind k)`
- L949 `int InventoryItemRowCount(void* panel)`: Rows currently in the inventory's item listbox.
- L961 `void MonitorInventoryListPopulation()`
- L992 `void MonitorDialogReplies()`
- L1195 `void TickGeneralMonitors()`
- L1220 `void* FindActiveSubScreenPanel()`
- L1242 `bool IsInGameSubScreenKind(PanelKind k)`
