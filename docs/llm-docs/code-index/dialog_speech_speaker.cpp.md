# dialog_speech_speaker.cpp (389 lines)

Source: `patches/Accessibility/dialog_speech_speaker.cpp`

Who is speaking a dialog line or bark, and whether their subtitle is redundant next to their voice-over.

## Declarations (in source order)

- L196 `bool IsHumanAppearance(int appearanceType)`
- L218 `bool IsNeverSuppressTag(const char* tag)`
- L227 `bool IsAlwaysSuppressTag(const char* tag)`
- L239 `bool FillSpeakerFromServerObject(void* partner, SpeakerInfo& out)`: Given a candidate server object, validate it's a creature and fill the tag / race / appearance fields.
- L276 `uint32_t ReadGuiDialogSpeakerClientId()`: Read CGuiInGame+0x170 — the current dialog entry's speaker as a CLIENT object id.
- L305 `bool IsSuppressibleSpeaker(const SpeakerInfo& info)`: True iff this speaker's subtitle is redundant given the VO, so we suppress it under the (shared) HumanSubtitles toggle.
- L311 `bool ResolveDialogSpeaker(SpeakerInfo& out)`
- L364 `bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out)`: Resolve the speaker of a bark bubble from its own owner field, rather than the dialog-speaker chain (which is empty for barks — see ResolveDialogSpeaker note above, where bark resolution always failed and every bark w...
