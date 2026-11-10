// Internal to the menus_modsettings module (menus_modsettings*.cpp); no other file includes
// it. The public interface is menus_modsettings.h. Here: what the files share.

#pragma once

#include "menus_modsettings.h"

#include <windows.h>
#include <shellapi.h>   // ShellExecuteA — the link rows' browser handoff
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "audio_cues.h"       // NavCue + GetNavCueResref
#include "audio_loop.h"       // LoopSource — glossary audition via CExoSoundSource
#include "strings.h"

#pragma comment(lib, "shell32.lib")  // ShellExecuteA

namespace acc::menus::modsettings {

// Toggle state. In-memory only — persistence is a follow-up.
//
// Defaults per user 2026-05-27 (ExtendedCycling semantics revised 2026-06-14):
//   - ExtendedCycling = OFF. In-world `,`/`.` are ALWAYS live; OFF means the
//     cycle is restricted to the DISCOVERY tier (objects the player has
//     organically had narrated — see discovery.*). Turning it ON widens the
//     cycle to everything in the area (the old "extended/spoilery" listing).
//     The filter lives in cycle_state::BuildCategoryListing.
//   - RoomShapes      = ON. Preserves the current corridor / junction /
//     plaza announce flow on cluster transitions.
//   - AudioGlossary   = unused slot (RowKind::Submenu, not a toggle).
// Index order MUST match the Option enum.
extern bool s_toggles[static_cast<int>(Option::Count)];

// Submenu state. `s_open` flips on OpenSubMenu / Close; `s_focused`
// is the currently-selected option index while open. `s_parentPanel`
// is the panel the user came from so Close can rebind the chain.
// `s_fgAtOpen` is the engine-foreground panel captured at OpenSubMenu
// time — used as the divergence baseline so we don't false-fire on
// stack configurations that are stable from the user's perspective.
// (In-game Options leaves the InGameMenu strip at panels[top]; the
// strip never equals our parent but the user is still "in" the panel
// they selected.)
//
// s_glossaryOpen flips when the user presses Enter on the
// AudioGlossary row. While true, HandleInput routes Up/Down/Enter/Esc
// to the glossary-specific handlers; Esc returns to the root submenu
// rather than the parent panel. s_glossaryFocused is the per-mode
// focus index (independent of the root focus so the user returns to
// the AudioGlossary row when they Esc out).
//
extern bool   s_open;
extern int    s_focused;
extern void*  s_parentPanel;
extern void*  s_fgAtOpen;
extern bool   s_glossaryOpen;
extern int    s_glossaryFocused;
// True while the nested keybind configurator (menus_keybinds) owns input.
// Unlike the glossary it keeps its own state in that TU; we only track that we
// routed into it so HandleInput / Tick / Close can defer to it.
extern bool   s_keybindsOpen;

// Pending glossary playback. Stamped by Enter on a glossary row;
// drained by Tick() once GetTickCount() ≥ s_pendingFireAt. The pause
// exists so the row name announcement ("Wall") finishes before the cue
// plays — without it the speech bleeds over the sample and the user
// can't cleanly correlate name with sound. 750 ms tuned by feel
// 2026-05-27: long enough to clear the speech tail at default rate,
// short enough that it doesn't feel laggy.
extern bool                s_pendingValid;
extern DWORD               s_pendingFireAt;
extern acc::audio::NavCue  s_pendingCue;
constexpr DWORD     kGlossaryDelayMs = 750;

// Audition playback handle. The glossary fires its sample through a
// self-managed CExoSoundSource (non-looping, 2D) rather than
// CExoSound::PlayOneShotSound. The one-shot mixer path is muted while the
// in-game menu pauses the world (engine SetSoundMode); a directly-driven
// source uses the same channel the GUI's own click sounds use, which stays
// audible under that pause. Reused across fires — each Start() stops the
// prior sample first; Close() stops any in-flight tail.
extern acc::audio::LoopSource s_glossaryPreview;

enum class RowKind {
    Toggle,    // Enter flips s_toggles[idx]; speech reads "Name: state"
    Submenu,   // Enter opens a nested view; speech reads "Name" only
    Slider,    // Left/Right adjust a percent; speech reads "Name: N percent"
    Link,      // Enter opens a URL in the browser; speech reads "Name" only
};

struct OptionSpec {
    Option              option;
    acc::strings::Id    label;
    RowKind             kind;
};

// Every row's label + kind, indexed BY Option (order must stay in lock-step
// with the enum — Spec() indexes straight into it). Where a row is *seen* is
// decided by the display tables below, not by this one.
constexpr OptionSpec k_options[] = {
    { Option::ExtendedCycling, acc::strings::Id::ModSettingExtendedCycling, RowKind::Toggle  },
    { Option::RoomShapes,      acc::strings::Id::ModSettingRoomShapes,      RowKind::Toggle  },
    { Option::HumanSubtitles,  acc::strings::Id::ModSettingHumanSubtitles,  RowKind::Toggle  },
    { Option::QEAllAround,     acc::strings::Id::ModSettingQEAllAround,     RowKind::Toggle  },
    { Option::TurretAutoAim,   acc::strings::Id::ModSettingTurretAutoAim,   RowKind::Toggle  },
    { Option::SkipIntros,      acc::strings::Id::ModSettingSkipIntros,      RowKind::Toggle  },
    { Option::CueVolume,       acc::strings::Id::ModSettingCueVolume,       RowKind::Slider  },
    { Option::Keybindings,     acc::strings::Id::KeybindsRootLabel,         RowKind::Submenu },
    { Option::AudioGlossary,   acc::strings::Id::ModSettingAudioGlossary,   RowKind::Submenu },
    { Option::SupportModder,   acc::strings::Id::ModSettingSupportModder,   RowKind::Link    },
    { Option::LatestChanges,   acc::strings::Id::ModSettingLatestChanges,   RowKind::Link    },
    { Option::OpenReadme,      acc::strings::Id::ModSettingOpenReadme,      RowKind::Link    },
};
constexpr int k_optionCount = static_cast<int>(
    sizeof(k_options) / sizeof(k_options[0]));
static_assert(k_optionCount == static_cast<int>(Option::Count),
              "k_options must cover every Option enumerator");

// ---- Display table ---------------------------------------------------------
//
// Top-to-bottom order of the list.
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
constexpr int k_rootRowCount = static_cast<int>(
    sizeof(k_rootRows) / sizeof(k_rootRows[0]));

// Every option is a row of the list: a row added to the enum but not here
// would be invisible with nothing else to catch it.
static_assert(k_rootRowCount == k_optionCount,
              "every Option must appear in the display table");

// Helpers shared across the files, by the file that defines them.

// menus_modsettings.cpp
const OptionSpec& Spec(Option opt);
Option FocusedOption();
const char* PersistKey(Option opt);
void SpeakFocusedOption(bool interrupt = true);

// menus_modsettings_glossary.cpp
void CancelPendingGlossaryCue();
void OpenGlossarySubMenu();
bool HandleInputGlossary(int keyCode);

// menus_modsettings_list.cpp
bool HandleInputList(int keyCode);

}  // namespace acc::menus::modsettings
