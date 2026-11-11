# code_patch.h (18 lines)

Source: `patches/Accessibility/code_patch.h`

Writing machine code into the game or into a trampoline of our own.

## Declarations (in source order)

- L11 `inline void WriteJmpRel32(uint8_t* at, uintptr_t target)`: A 5-byte JMP rel32 at `at` to `target`.
