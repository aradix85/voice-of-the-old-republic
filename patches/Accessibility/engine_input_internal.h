// Internal to the engine_input module (engine_input*.cpp); no other file includes
// it. The public interface is engine_input.h. Here: what the files share.

#pragma once

#include "engine_input.h"

#include "engine_addr.h"

#include <cstdint>

namespace acc::engine {

// Helpers shared across the files, by the file that defines them.

// engine_input.cpp
// The CExoInput facade, or nullptr when the engine has none yet or the global
// is unknown on this build.
void* ExoInput();

}  // namespace acc::engine
