// Internal to the engine_panels module (engine_panels*.cpp); no other file
// includes it. The public interface is engine_panels.h.

#pragma once

#include "engine_panels.h"

#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

// The first links of the CGuiInGame chain (the AppManager walk).
#include "engine_app.h"

namespace acc::engine {

// Last link of the chain: CClientExoApp internal -> CGuiInGame
// (docs/llm-docs/re/swkotor.exe.h). The same on KOTOR 2, where the forwarder
// at 0x0078C330 does `MOV ECX,[this+0x40]` and calls the CGuiInGame panel
// creator.
inline constexpr size_t    kClientExoAppGuiInGameOff = acc::off::Same(0x40);

// CGuiInGame -> CSWGuiMainInterface. KOTOR 2's SetSWGuiStatus @0x007C9C40
// adds/removes the [this+0x98] panel exactly where KOTOR 1's uses
// main_interface (+0x90).
inline const size_t        kGuiInGameMainInterfaceOff = acc::off::Pick(0x90, 0x98);

// engine_panels_reads.cpp: the CGuiInGame*, or nullptr.
void* ResolveGuiInGame();

// engine_panels_vtables.cpp: the kind of a slotless panel by its vtable, or
// Unknown.
PanelKind IdentifyByVtable(void* panel);

}  // namespace acc::engine
