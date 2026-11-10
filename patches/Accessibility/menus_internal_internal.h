// Internal to the menus_internal module (menus_internal*.cpp); no other file includes
// it. The public interface is menus_internal.h. Here: what the files share.

#pragma once

#include "menus_internal.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_panels.h"   // HasVtable

using namespace acc::engine;

namespace acc::menus::detail {

// Helpers shared across the files, by the file that defines them.

}  // namespace acc::menus::detail
