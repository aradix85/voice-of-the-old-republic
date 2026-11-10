// Internal to the engine_panels module (engine_panels*.cpp); no other file includes
// it. The public interface is engine_panels.h. Here: what the files share.

#pragma once

#include "engine_panels.h"

#include <windows.h>  // SEH __try / __except
#include <cstddef>
#include <cstdint>
#include <cstring>

// The first links of the CGuiInGame chain come from engine_app.h, which owns
// the whole AppManager walk (Phase-3 B1). This header used to carry its own
// copies under names one letter different from the canonical ones
// ("...ClientOff" vs "...ClientAppOffset"), which is how the same address
// ended up declared in three places — a Phase-1 split that moved constants
// verbatim rather than consolidating them.
#include "engine_app.h"

namespace acc::engine {

// Last link of the chain, specific to the panels module: CClientExoApp
// internal -> CGuiInGame. Verified against the struct definitions in
// docs/llm-docs/re/swkotor.exe.h.
//
// Identical on KOTOR 2, and directly witnessed there (2026-08-01): the
// forwarder at 0x0078C330 does `MOV ECX,[this+0x40]` and calls the CGuiInGame
// panel creator, so this offset is what produces the very object the K2 slot
// table was read out of. See engine_app.h's note on the chain above it.
inline constexpr size_t    kClientExoAppGuiInGameOff = acc::off::Same(0x40);

// CGuiInGame -> CSWGuiMainInterface. Canonical home for what engine_radial,
// engine_actionbar, engine_picker and combat_diag each declared privately.
// KOTOR 2 value witnessed in SetSWGuiStatus @0x007C9C40, which adds/removes
// the [this+0x98] panel exactly where KOTOR 1's uses main_interface (+0x90).
inline const size_t        kGuiInGameMainInterfaceOff = acc::off::Pick(0x90, 0x98);

// Walk that chain and return the CGuiInGame*. Defined in
// engine_panels.cpp next to the identity registry; the state readers in
// engine_panels_state.cpp are its only callers outside that file.
void* ResolveGuiInGame();

// Helpers shared across the files, by the file that defines them.

// engine_panels_vtables.cpp
bool IsSaveLoadStructural(void* panel);
bool IsWorkbenchUpgradeStructural(void* panel);
bool IsInGameMessagesStructural(void* panel);
bool IsWorkbenchItemsStructural(void* panel);
bool IsWorkbenchSelectStructural(void* panel);
bool IsWorkbenchCreateItemStructural(void* panel);
bool IsWorkbenchCreateMedicalStructural(void* panel);
bool IsLevelUpStructural(void* panel);
bool IsCharGenStructural(void* panel);
bool IsMainMenuOptionsStructural(void* panel);
bool IsMainMenuStructural(void* panel);
bool IsPazaakStartStructural(void* panel);
bool IsPazaakWagerStructural(void* panel);
bool IsQuestItemStructural(void* panel);
bool IsScriptSelectStructural(void* panel);
bool IsPowersLevelUpStructural(void* panel);
PanelKind IdentifyOptionsSubScreen(void* panel);

}  // namespace acc::engine
