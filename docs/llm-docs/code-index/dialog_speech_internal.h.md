# dialog_speech_internal.h (53 lines)

Source: `patches/Accessibility/dialog_speech_internal.h`

Internal to the dialog_speech module (dialog_speech*.cpp); no other file includes it. The public interface is dialog_speech.h. Here: what the files share.

## Declarations (in source order)

- L35 `struct SpeakerInfo`: ---- Speaker resolution chain -------------------------------------------- PRIMARY (per-line, covers overheard NPC-to-NPC): gui = ResolveGuiInGame() (CGuiInGame*) ↓ +0x170 current_dialog_speaker (CLIENT object id) Cli...
- L45 `bool IsSuppressibleSpeaker(const SpeakerInfo& info)`: dialog_speech_speaker.cpp
- L46 `bool ResolveDialogSpeaker(SpeakerInfo& out)`
- L47 `bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out)`
- L50 `bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize)`: dialog_speech_marker.cpp
