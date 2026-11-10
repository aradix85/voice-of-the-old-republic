# dialog_speech_speaker.cpp (257 lines)

Source: `patches/Accessibility/dialog_speech_speaker.cpp`

Who is speaking a dialog line or bark, and whether their subtitle is redundant next to their voice-over.

## Declarations (in source order)

- L112 `bool IsHumanAppearance(int appearanceType)`
- L130 `bool TagListed(const char* const (&list)[N], const char* tag)`
- L140 `void* ServerObjectFromClientId(uint32_t clientId)`: The creature behind a CLIENT object id, or nullptr.
- L150 `bool FillSpeakerFromServerObject(void* partner, SpeakerInfo& out)`: Given a candidate server object, validate it's a creature and fill the tag / race / appearance fields.
- L184 `uint32_t ReadGuiDialogSpeakerClientId()`: The current dialog line's speaker as a CLIENT object id, off CGuiInGame; kInvalidObjectId or 0 when there is none.
- L203 `bool IsSuppressibleSpeaker(const SpeakerInfo& info)`: Human-appearance speakers, and droids: either Basic VO (HK-47) or beeps whose subtitle carries no meaning (T3-M4; the meaning lives in the reply choices, which are read separately).
- L209 `bool ResolveDialogSpeaker(SpeakerInfo& out)`
- L241 `bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out)`: A bark names its owner itself (CSWGuiBarkBubble.object_id, a CLIENT id, written by SetBark); the dialog speaker chain is empty for barks.
