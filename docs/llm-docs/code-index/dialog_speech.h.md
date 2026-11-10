# dialog_speech.h (15 lines)

Source: `patches/Accessibility/dialog_speech.h`

Dialog narration, polled each tick: the NPC line (silenced where the speaker's voice-over already says it, unless the HumanSubtitles toggle is on), a computer terminal's output, the security-camera feed's hint, and bark bubbles. The replies are read through the listbox specs (menus_listbox.cpp).

## Declarations (in source order)

- L12 `void Tick()`: Idle when no dialog panel is mounted.
