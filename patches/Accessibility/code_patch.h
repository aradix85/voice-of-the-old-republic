// Writing machine code into the game or into a trampoline of our own.

#pragma once

#include <cstdint>
#include <cstring>

namespace acc::code_patch {

// A 5-byte JMP rel32 at `at` to `target`. The memory must be writable.
inline void WriteJmpRel32(uint8_t* at, uintptr_t target) {
    at[0] = 0xe9;
    const int32_t rel = static_cast<int32_t>(target - (reinterpret_cast<uintptr_t>(at) + 5));
    std::memcpy(at + 1, &rel, 4);
}

}  // namespace acc::code_patch
