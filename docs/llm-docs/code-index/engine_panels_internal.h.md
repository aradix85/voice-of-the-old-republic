# engine_panels_internal.h (37 lines)

Source: `patches/Accessibility/engine_panels_internal.h`

Internal to the engine_panels module (engine_panels*.cpp); no other file includes it. The public interface is engine_panels.h.

## Declarations (in source order)

- L30 `void* ResolveGuiInGame()`: engine_panels_reads.cpp: the CGuiInGame*, or nullptr.
- L34 `PanelKind IdentifyByVtable(void* panel)`: engine_panels_vtables.cpp: the kind of a slotless panel by its vtable, or Unknown.

Also 2 constants or variables at namespace scope.
