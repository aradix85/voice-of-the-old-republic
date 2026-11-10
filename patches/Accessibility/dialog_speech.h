// Dialog narration, polled each tick: the NPC line (silenced where the
// speaker's voice-over already says it, unless the HumanSubtitles toggle is
// on), a computer terminal's output, the security-camera feed's hint, and
// bark bubbles. The replies are read through the listbox specs
// (menus_listbox.cpp).

#pragma once

namespace acc::dialog_speech {

// Idle when no dialog panel is mounted.
void Tick();

}  // namespace acc::dialog_speech
