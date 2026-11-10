# dialog_speech.cpp (910 lines)

Source: `patches/Accessibility/dialog_speech.cpp`

transient (engine LYT loader use-after-free)

## Declarations (in source order)

- L105 `bool IsHumanAppearance(int appearanceType)`
- L217 `bool IsNeverSuppressTag(const char* tag)`
- L268 `bool IsAlwaysSuppressTag(const char* tag)`
- L292 `bool IsSuppressibleSpeaker(const SpeakerInfo& info)`: True iff this speaker's subtitle is redundant given the VO, so we suppress it under the (shared) HumanSubtitles toggle.
- L314 `char FoldByte(unsigned char c)`: Lowercase ASCII for matching; other bytes pass through.
- L321 `bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize)`: If `line` opens with a bracketed skill-check outcome marker, copy the marker's inner text (verbatim, brackets removed, trailing whitespace trimmed) into `out` and return true.
- L359 `bool FillSpeakerFromServerObject(void* partner, SpeakerInfo& out)`: Given a candidate server object, validate it's a creature and fill the tag / race / appearance fields.
- L396 `uint32_t ReadGuiDialogSpeakerClientId()`: Read CGuiInGame+0x170 — the current dialog entry's speaker as a CLIENT object id.
- L408 `bool ResolveDialogSpeaker(SpeakerInfo& out)`
- L461 `bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out)`: Resolve the speaker of a bark bubble from its own owner field, rather than the dialog-speaker chain (which is empty for barks — see ResolveDialogSpeaker note above, where bark resolution always failed and every bark w...
- L492 `DialogPanelMatch FindActiveDialogPanel()`
- L530 `void* FindBarkBubblePanel()`: Find a BarkBubble panel (independent lifecycle from main dialog).
- L535 `int ReadListBoxRowCount(void* panel, size_t lbOffset)`: Read row count of a CSWGuiListBox at panel + offset.
- L553 `bool ReadFirstVisibleText(void* panel, char* outBuf, size_t bufSize)`: Walk a panel's controls[] for the first child whose text resolves non-empty.
- L590 `void TickComputerRows(void* panel)`: For the Computer dialog variant, also speak the message_listbox (terminal output) when its row count grows.
- L640 `void TickDialogPanel()`: ---- NPC line + replies count for the main dialog panel.
- L796 `void TickCameraFeed()`: ---- Security-camera feed — its own panel, its own lifecycle.
- L824 `void TickBark()`: ---- BarkBubble — independent lifecycle.
- L898 `void Tick()`
