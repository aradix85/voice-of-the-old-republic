// Internal to the engine_input module (engine_input*.cpp); no other file includes
// it. The public interface is engine_input.h. Here: what the files share.

#pragma once

#include "engine_input.h"

#include <windows.h>
#include <intrin.h>   // _ReturnAddress — names the failing mouse-init step
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_rebase.h"

namespace acc::engine {

// Engine global holding the CExoInput facade pointer (the same symbol
// HideLoadScreen reads before calling SetActive). Verified in Lane's DB:
// SYMBOL ExoInput @0x007a39e4.
//
// KOTOR 2 @0x00a1b48c, resolved 2026-08-13 (it sat on TodoGlobal until then,
// so BOTH constants here were 0 on KOTOR 2 and every reacquire/release faulted
// into the SEH handler below — 25 "exception during SetActive" lines in one
// short session, i.e. the focus-theft fix did not exist there). Two
// independent witnesses agree:
//   * the constructor at 0x0072f760 writes the CExoInput RTTI vtable
//     (0x0099cab8) into the new object, and 0x00409c7e stores that object to
//     0x00a1b48c;
//   * the engine's global-pointer cluster keeps KOTOR 1's relative layout
//     (ExoResMan/GuiMan/Aurora/AppMan/VM/Tlk/Rules deltas all match), and
//     KOTOR 1's ExoInput is likewise the dword below ExoResMan.
const uintptr_t kAddrExoInputGlobal =
    acc::addr::PickGlobal(0x007a39e4, 0x00a1b48c);

// Helpers shared across the files, by the file that defines them.

}  // namespace acc::engine
