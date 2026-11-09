# menus_extract.cpp (330 lines)

Source: `patches/Accessibility/menus_extract.cpp`

control text extraction (the announce ladder).

FromControl and its three extract-only helpers (FindSiblingLabel, IsCycleFlankerArrow, LookupCycleCategory) lifted out of menus.cpp. Helpers shared with chain navigation (IsChainNavigable, IsClassSelectionIcon, ClassLabelCache*, GetControlCenter) and the focused-panel pointer (g_currentPanel) cross the seam via menus_internal.h.

FromControl itself is the ladder: it does no extraction of its own, it runs the Try* steps in order and applies the three announce suffixes. Each step's rationale lives at that step's definition in the anonymous namespace below, under the number it had when the ladder was one function.

## Declarations (in source order)

- L41 `const char* LookupCycleCategory(void* control)`
- L62 `void ApplyCycleCategoryPrefix(void* control, char* outBuf, size_t bufSize)`: Cycle value-display prefix.
- L74 `void AppendToggleSuffix(void* control, char* outBuf, size_t bufSize)`: Append element-state suffix for toggles.
- L111 `void AppendDisabledSuffix(void* control, void* ownerPanel, char* outBuf, size_t bufSize)`: Append "nicht verfügbar" / "unavailable" suffix when the focused button is engine-disabled.
- L173 `bool EmitText(char* outBuf, size_t bufSize, const char* text)`: Hand `text` back to the caller, TRUNCATING when it does not fit.
- L208 `void* ResolveOwnerPanel(void* control, void* ownerPanel)`: Resolve the panel that owns `control`.
- L221 `bool IsPanelDirectChild(void* panel, void* control)`: True when `control` sits directly in panel.controls[].
- L235 `void ResetCycleCategoryCache()`
- L239 `void CaptureCycleCategory(void* control, const char* category)`
- L260 `const char* FromControl(void* control, char* outBuf, size_t bufSize, void* ownerPanel)`
