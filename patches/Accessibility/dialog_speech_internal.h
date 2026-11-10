// Internal to the dialog_speech module (dialog_speech*.cpp); no other file includes
// it. The public interface is dialog_speech.h. Here: what the files share.

#pragma once

#include "dialog_speech.h"

#include <windows.h>
#include <cstdint>
#include <cstring>


namespace acc::dialog_speech {

// ---- Speaker resolution chain --------------------------------------------
//
// PRIMARY (per-line, covers overheard NPC-to-NPC):
//   gui = ResolveGuiInGame()  (CGuiInGame*)
//     ↓ +0x170  current_dialog_speaker (CLIENT object id)
//   ClientToServerObjectId → ResolveServerObjectHandle → CSWSObject*
//
// FALLBACK (1-on-1 dialog + barks where the GUI field isn't populated):
//   player = GetPlayerServerCreature()  (CSWSCreature*)
//     ↓ +0x54  dialog_owner (SERVER object id)
//   ResolveServerObjectHandle → CSWSObject*
//
// Either way we land on a CSWSObject*, validate kind == Creature(5), then
// read +0xa74 stats → +0x186 appearance / +0xdc race. The GUI field is
// authoritative for the *current line's* speaker; the player-partner field
// only ever names the player's own conversation partner, so it's wrong in
// multi-party / overheard scenes — hence GUI-first.
//
// Returns false if no link resolves, the object isn't a creature, or SEH
// faults — caller treats false as "unknown speaker, default to speaking".
struct SpeakerInfo {
    void* speaker;          // CSWSCreature* (validated via kind=Creature)
    int   appearance;       // appearance.2da row index, -1 if unread
    int   raceEnum;         // RACE enum (HUMAN=6, DROID=5, 0 otherwise)
    char  tag[32];          // CSWSObject.tag (or "" if unread)
};

// Helpers shared across the files, by the file that defines them.

// dialog_speech_speaker.cpp
bool IsSuppressibleSpeaker(const SpeakerInfo& info);
bool ResolveDialogSpeaker(SpeakerInfo& out);
bool ResolveBarkSpeaker(void* barkPanel, SpeakerInfo& out);

// dialog_speech_marker.cpp
bool ExtractSkillCheckMarker(const char* line, char* out, size_t outSize);

}  // namespace acc::dialog_speech
