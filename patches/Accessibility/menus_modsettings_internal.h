// Internal to the menus_modsettings module (menus_modsettings*.cpp); no other file includes
// it. The public interface is menus_modsettings.h. Here: what the files share.

#pragma once

#include "menus_modsettings.h"

#include "audio_loop.h"
#include "strings.h"

namespace acc::menus::modsettings {

using Id = acc::strings::Id;

enum class RowKind {
    Toggle,   // Enter flips the bit; spoken "Name: state"
    Submenu,  // Enter opens a nested view; spoken "Name"
    Slider,   // Left/Right adjust a percent; spoken "Name: N percent"
    Link,     // Enter opens a URL in the browser; spoken "Name"
};

struct OptionSpec {
    Option      option;
    Id          label;
    RowKind     kind;
    const char* persistKey;  // acc_settings.ini key, or nullptr when not persisted here
};

// Indexed by Option. SkipIntros keeps its state in the filesystem and the cue
// volume in audio_bus, so neither has a key here.
constexpr OptionSpec k_options[] = {
    { Option::ExtendedCycling, Id::ModSettingExtendedCycling, RowKind::Toggle, "ExtendedCycling" },
    { Option::RoomShapes,      Id::ModSettingRoomShapes,      RowKind::Toggle, "RoomShapes" },
    { Option::HumanSubtitles,  Id::ModSettingHumanSubtitles,  RowKind::Toggle, "HumanSubtitles" },
    { Option::QEAllAround,     Id::ModSettingQEAllAround,     RowKind::Toggle, "QEAllAround" },
    { Option::TurretAutoAim,   Id::ModSettingTurretAutoAim,   RowKind::Toggle, "TurretAutoAim" },
    { Option::SkipIntros,      Id::ModSettingSkipIntros,      RowKind::Toggle,  nullptr },
    { Option::CueVolume,       Id::ModSettingCueVolume,       RowKind::Slider,  nullptr },
    { Option::Keybindings,     Id::KeybindsRootLabel,         RowKind::Submenu, nullptr },
    { Option::AudioGlossary,   Id::ModSettingAudioGlossary,   RowKind::Submenu, nullptr },
    { Option::SupportModder,   Id::ModSettingSupportModder,   RowKind::Link,    nullptr },
    { Option::LatestChanges,   Id::ModSettingLatestChanges,   RowKind::Link,    nullptr },
    { Option::OpenReadme,      Id::ModSettingOpenReadme,      RowKind::Link,    nullptr },
};
constexpr int k_optionCount = static_cast<int>(sizeof(k_options) / sizeof(k_options[0]));
static_assert(k_optionCount == static_cast<int>(Option::Count),
              "k_options must cover every Option enumerator");

constexpr bool OptionsInEnumOrder() {
    for (int i = 0; i < k_optionCount; ++i) {
        if (static_cast<int>(k_options[i].option) != i) return false;
    }
    return true;
}
static_assert(OptionsInEnumOrder(), "k_options must follow the Option enum");

// The list from top to bottom.
constexpr Option k_rootRows[] = {
    Option::ExtendedCycling,
    Option::QEAllAround,
    Option::RoomShapes,
    Option::HumanSubtitles,
    Option::TurretAutoAim,
    Option::SkipIntros,
    Option::CueVolume,
    Option::Keybindings,
    Option::AudioGlossary,
    Option::SupportModder,
    Option::LatestChanges,
    Option::OpenReadme,
};
constexpr int k_rootRowCount = static_cast<int>(sizeof(k_rootRows) / sizeof(k_rootRows[0]));
static_assert(k_rootRowCount == k_optionCount, "every Option must appear in the list");

// The GUI sounds' priority group. The in-game menu's SetSoundMode pause spares
// groups 1, 2 and 0xb, so a preview on it stays audible there.
constexpr int kGuiPriorityGroup = 0x0b;

// Shared state, by the file that defines it.

// menus_modsettings.cpp
extern bool s_toggles[static_cast<int>(Option::Count)];
extern int  s_focused;       // row of the list under the cursor
extern bool s_keybindsOpen;  // the keybind configurator (menus_keybinds) owns input

// menus_modsettings_glossary.cpp
extern bool s_glossaryOpen;
// The glossary's and the cue-volume slider's preview: a self-managed 2D source,
// since the one-shot mixer path is muted while the in-game menu pauses the world.
extern acc::audio::LoopSource s_preview;

// Helpers shared across the files, by the file that defines them.

// menus_modsettings.cpp
const OptionSpec& Spec(Option opt);  // callers rule out Option::Count first
Option FocusedOption();              // Option::Count when the cursor is out of range
void SpeakFocusedOption(bool interrupt = true);
// Up/Down/Home/End on a list of `count` rows, clamped; false for any other key.
bool StepListCursor(int keyCode, int count, int* cursor);

// menus_modsettings_glossary.cpp
void OpenGlossarySubMenu();
void ResetGlossary();  // closed, cursor on the first row, no audition queued
bool HandleInputGlossary(int keyCode);
void TickGlossary();

// menus_modsettings_list.cpp
bool HandleInputList(int keyCode);

}  // namespace acc::menus::modsettings
