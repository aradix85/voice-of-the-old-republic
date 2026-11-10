// The virtual "Mod settings" submenu, reached through an entry the chain adds
// to the in-game and the title-screen Options panels.
//
// The entry is not an engine control: its `control` is a sentinel pointer
// (IsRootAnchor), which menus_extract speaks as "Mod settings" and the Enter
// dispatcher opens with OpenSubMenu. The submenu has no engine panel either;
// while IsOpen(), the input hook routes keys through HandleInput first. The
// audio glossary and the keybind configurator are nested views; Esc inside one
// returns to the list.

#pragma once

#include <cstddef>

namespace acc::menus::modsettings {

// A row's stable identity (persistence keys, GetToggle callers), not its place
// on screen. A new option needs an enumerator, a label, a row in the option
// table and a place in the list (menus_modsettings_internal.h).
enum class Option {
    ExtendedCycling = 0,
    RoomShapes,
    HumanSubtitles,
    QEAllAround,      // Q/E cycle everything around the player (qe_ring)
    TurretAutoAim,    // the turret auto-tracks the selected fighter
    SkipIntros,       // state is biologo.bik vs biologo.bik.disabled
    CueVolume,        // slider; audio_bus owns the value
    Keybindings,      // submenu: the keybind configurator (menus_keybinds)
    AudioGlossary,    // submenu
    SupportModder,    // link rows: open a page in the default browser
    LatestChanges,
    OpenReadme,
    Count
};

// True iff `control` is the root entry's sentinel.
bool IsRootAnchor(void* control);

// Calls `callback` once with the root sentinel and its sort position on
// InGameOptions and MainMenuOptions (the last stop of the chain); nothing for
// other panels.
void ForEachRootAnchor(void* panel,
                       bool (*callback)(void* sentinel, int sortCx, int sortCy,
                                        void* userData),
                       void* userData);

// The root entry's speech ("Mod settings"); false only for a degenerate buffer.
bool ExtractRootLabel(char* outBuf, size_t bufSize);

// Opens the submenu over `parentPanel` and speaks the title and first row.
void OpenSubMenu(void* parentPanel);

bool IsOpen();

// Closes the submenu and rebinds the chain on the parent panel; no-op when
// closed.
void Close();

// Press edges only. True when the submenu consumed the key; false when it is
// closed, or for keys it leaves to the engine (the global hotkeys).
bool HandleInput(int keyCode);

// The state of a toggle option; false for out-of-range and non-toggle rows.
bool GetToggle(Option option);

// Per-frame: the keybind configurator's capture poll and the glossary's
// delayed audition. Cheap when neither is active.
void Tick();

}  // namespace acc::menus::modsettings
