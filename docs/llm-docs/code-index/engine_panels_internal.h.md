# engine_panels_internal.h (48 lines)

Source: `patches/Accessibility/engine_panels_internal.h`

internal seam between engine_panels.cpp and engine_panels_state.cpp.

This is NOT public API. engine_panels.h stays the public surface; this header exists only because the Phase-1 structure pass (refactoring candidate 6) split the panel-identity registry from the foreground/UI-blocking + input-class state readers, and both halves walk the same CGuiInGame resolution chain.

Values are unchanged RE facts, moved verbatim from engine_panels.cpp.

## Declarations (in source order)

- L45 `void* ResolveGuiInGame()`: Walk that chain and return the CGuiInGame*.

Also 2 constants or variables at namespace scope.
