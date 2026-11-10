# menus_internal.cpp (134 lines)

Source: `patches/Accessibility/menus_internal.cpp`

Small control helpers the menus files share: GUI threshold scaling, a control's centre and its lookup by id, chain navigability, the save/load panel test and entry read, and queued activation.

## Declarations (in source order)

- L27 `int ScaleGuiThresholdPx(int guiPx)`
- L42 `bool GetControlCenter(void* control, int& outCx, int& outCy)`: A degenerate extent (zero or negative width or height) shows up on hidden panels and templated control prototypes.
- L63 `bool IsChainNavigable(void* control)`: A button (CSWGuiButton or its subclasses CharButton / ActivatedButton / ButtonToggle) or a slider.
- L73 `void* FindControlById(void* panel, int id)`: The .gui ids are stable per panel kind, unlike the control's text (which changes with the language) or its index in panel.controls.
- L98 `bool IsSaveLoadPanel(void* panel)`: The panel is allocated when the user picks Load or Save and has no slot in CGuiInGame, so its ctor-written vtable is its identity.
- L106 `const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: The save-row fields (areaname, lastmodule) are plain CExoStrings from the save GFF, valid until the entry is freed.
- L119 `bool QueueControlActivate(void* target, const char* logPrefix)`: The activate itself runs one tick later in TickPendingOps.
