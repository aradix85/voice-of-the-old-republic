# dialog_speech_internal.h (35 lines)

Source: `patches/Accessibility/dialog_speech_internal.h`

Internal to the dialog_speech module (dialog_speech*.cpp); no other file includes it. The public interface is dialog_speech.h.

## Declarations (in source order)

- L15 `struct SpeakerInfo`: The speaker of a line or bark, a creature.
- L24 `bool IsSuppressibleSpeaker(const SpeakerInfo& info)`: dialog_speech_speaker.cpp.
- L29 `bool ResolveDialogSpeaker(SpeakerInfo& out)`: The current dialog line's speaker: the GUI's per-line speaker first, then the player's own conversation partner.
- L32 `bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out)`: The owner of a bark bubble.
