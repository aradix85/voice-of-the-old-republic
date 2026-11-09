# dialog_speech.cpp (924 lines)

Source: `patches/Accessibility/dialog_speech.cpp`

transient (engine LYT loader use-after-free)

## Declarations (in source order)

- L105 `bool IsHumanAppearance(int appearanceType)`
- L217 `bool IsNeverSuppressTag(const char* tag)`
- L268 `bool IsAlwaysSuppressTag(const char* tag)`
- L292 `bool IsSuppressibleSpeaker(const SpeakerInfo& info)`: True iff this speaker's subtitle is redundant given the VO, so we suppress it under the (shared) HumanSubtitles toggle.
- L328 `char FoldByte(unsigned char c)`: Fold one CP1252 byte (CExoString encoding) to lowercase ASCII for matching.
- L342 `bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize)`: If `line` opens with a bracketed skill-check outcome marker, copy the marker's inner text (verbatim, brackets removed, leading "\xA1" and trailing whitespace trimmed) into `out` and return true.
- L388 `bool FillSpeakerFromServerObject(void* partner, SpeakerInfo& out)`: Given a candidate server object, validate it's a creature and fill the tag / race / appearance fields.
- L425 `uint32_t ReadGuiDialogSpeakerClientId()`: Read CGuiInGame+0x170 — the current dialog entry's speaker as a CLIENT object id.
- L437 `bool ResolveDialogSpeaker(SpeakerInfo& out)`
- L490 `bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out)`: Resolve the speaker of a bark bubble from its own owner field, rather than the dialog-speaker chain (which is empty for barks — see ResolveDialogSpeaker note above, where bark resolution always failed and every bark w...
- L521 `DialogPanelMatch FindActiveDialogPanel()`
- L559 `void* FindBarkBubblePanel()`: Find a BarkBubble panel (independent lifecycle from main dialog).
- L564 `int ReadListBoxRowCount(void* panel, size_t lbOffset)`: Read row count of a CSWGuiListBox at panel + offset.
- L582 `bool ReadFirstVisibleText(void* panel, char* outBuf, size_t bufSize)`: Walk a panel's controls[] for the first child whose text resolves non-empty.
- L607 `void Tick()`
