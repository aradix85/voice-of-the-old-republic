# menus_extract.cpp (331 lines)

Source: `patches/Accessibility/menus_extract.cpp`

control text extraction (the announce ladder).

FromControl is the ladder: it does no extraction of its own, it runs the Try* steps in order (one file per kind of control, menus_extract_*.cpp) and applies the three announce suffixes. Each step's rationale lives at its definition, under its step number. Helpers shared with chain navigation (IsChainNavigable, IsClassSelectionIcon, ClassLabelCache*, GetControlCenter) and the focused-panel pointer (g_currentPanel) come from menus_internal.h.

## Declarations (in source order)

- L42 `const char* LookupCycleCategory(void* control)`
- L63 `void ApplyCycleCategoryPrefix(void* control, char* outBuf, size_t bufSize)`: Cycle value-display prefix.
- L75 `void AppendToggleSuffix(void* control, char* outBuf, size_t bufSize)`: Append element-state suffix for toggles.
- L112 `void AppendDisabledSuffix(void* control, void* ownerPanel, char* outBuf, size_t bufSize)`: Append "nicht verfügbar" / "unavailable" suffix when the focused button is engine-disabled.
- L174 `bool EmitText(char* outBuf, size_t bufSize, const char* text)`: Hand `text` back to the caller, TRUNCATING when it does not fit.
- L209 `void* ResolveOwnerPanel(void* control, void* ownerPanel)`: Resolve the panel that owns `control`.
- L222 `bool IsPanelDirectChild(void* panel, void* control)`: True when `control` sits directly in panel.controls[].
- L236 `void ResetCycleCategoryCache()`
- L240 `void CaptureCycleCategory(void* control, const char* category)`
- L261 `const char* FromControl(void* control, char* outBuf, size_t bufSize, void* ownerPanel)`
