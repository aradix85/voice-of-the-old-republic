// Internal to the dialog_speech module (dialog_speech*.cpp); no other file
// includes it. The public interface is dialog_speech.h.

#pragma once

#include "dialog_speech.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace acc::dialog_speech {

// The speaker of a line or bark, a creature.
struct SpeakerInfo {
    void* speaker    = nullptr;  // CSWSCreature*
    int   appearance = -1;       // appearance.2da row, -1 if unread
    int   raceEnum   = 0;        // RACE enum (HUMAN=6, DROID=5), 0 if unread
    char  tag[32]    = {};       // CSWSObject.tag, "" if unread
};

// dialog_speech_speaker.cpp. True when the speaker's voice-over already says
// the subtitle, so the HumanSubtitles rule may silence it.
bool IsSuppressibleSpeaker(const SpeakerInfo& info);

// The current dialog line's speaker: the GUI's per-line speaker first, then
// the player's own conversation partner. False when neither resolves to a
// creature, which the caller treats as "unknown, keep speaking".
bool ResolveDialogSpeaker(SpeakerInfo& out);

// The owner of a bark bubble. False for an ownerless bark.
bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out);

}  // namespace acc::dialog_speech
