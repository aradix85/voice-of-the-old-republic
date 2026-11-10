# menus_monitors.cpp (1244 lines)

Source: `patches/Accessibility/menus_monitors.cpp`

general per-tick monitors.

Post-Step-5 cleanup. See menus_monitors.h for the rationale and public surface. Function bodies are unchanged from the inline originals in menus.cpp; only namespacing and linkage changed.

## Declarations (in source order)

- L83 `bool acc::menus::detail::ParkCursorToCorner(const char* tag)`
- L108 `void AnnounceControl(void* control)`
- L199 `void MonitorFocusedControl()`
- L413 `uint32_t SpecStrref(const InGameSubScreenSpec* s)`: Per-game strref for a spec row.
- L418 `const InGameSubScreenSpec* FindSpec(PanelKind k)`
- L429 `bool IsSubScreenTracked(void* p)`
- L436 `void AnnounceNewSubScreens(void** panels, int count)`
- L540 `bool IsContentMonitored(PanelKind k)`
- L616 `bool IsContentSpoken(PanelKind k)`: Of the content-monitored kinds, which may SPEAK their diff.
- L626 `void BuildContentFingerprint(void* panel, char* out, size_t outSize)`
- L682 `bool FingerprintContainsSegment(const char* hay, size_t hayLen, const char* seg, size_t segLen)`
- L702 `void SpeakNewSegments(const char* prev, const char* curr)`
- L737 `char* GetContentSnapshot(void* panel)`
- L754 `void MonitorPanelContents()`
- L872 `unsigned ComputeReplySetSignature(int replyCount, int rowCount)`: FNV-1a over every active reply's text plus the count.
- L891 `bool ParkDialogCursorOffReplies(void* replyLb)`
- L901 `bool IsDialogPanelKind(PanelKind k)`
- L946 `int InventoryItemRowCount(void* panel)`: Rows currently in the inventory's item listbox.
- L958 `void MonitorInventoryListPopulation()`
- L989 `void MonitorDialogReplies()`
- L1192 `void TickGeneralMonitors()`
- L1217 `void* FindActiveSubScreenPanel()`
- L1239 `bool IsInGameSubScreenKind(PanelKind k)`
