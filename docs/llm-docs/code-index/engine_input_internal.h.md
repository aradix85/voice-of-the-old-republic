# engine_input_internal.h (22 lines)

Source: `patches/Accessibility/engine_input_internal.h`

Internal to the engine_input module (engine_input*.cpp); no other file includes it. The public interface is engine_input.h. Here: what the files share.

## Declarations (in source order)

- L19 `void* ExoInput()`: engine_input.cpp The CExoInput facade, or nullptr when the engine has none yet or the global is unknown on this build.
