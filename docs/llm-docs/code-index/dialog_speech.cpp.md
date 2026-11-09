# dialog_speech.cpp (952 lines)

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
- L533 `void SpeakSubtitle(const char* text, bool interrupt)`: ---- Channel decision ---------------------------------------------------- Every line this TU speaks IS a subtitle — the NPC line, its skill-check marker, the computer terminal's rows, the bark bubble, and the R re-sp...
- L549 `DialogPanelMatch FindActiveDialogPanel()`
- L587 `void* FindBarkBubblePanel()`: Find a BarkBubble panel (independent lifecycle from main dialog).
- L592 `int ReadListBoxRowCount(void* panel, size_t lbOffset)`: Read row count of a CSWGuiListBox at panel + offset.
- L610 `bool ReadFirstVisibleText(void* panel, char* outBuf, size_t bufSize)`: Walk a panel's controls[] for the first child whose text resolves non-empty.
- L635 `void Tick()`
