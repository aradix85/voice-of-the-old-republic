# diag_chargen_feats.h (23 lines)

Source: `patches/Accessibility/diag_chargen_feats.h`

Diagnostic: one-shot structural dump of CSWGuiFeatsCharGen panels.

Logs the four feat lists (existing / granted / available / chosen — the CExoUShortList fields the engine cycles through during chargen) plus the SkillFlowChart grid (row × col, each cell's featId + status + strref cross-reference). Dedups per panel pointer so re-focusing the same panel doesn't re-dump.

No state outside the per-pointer dedup. Silently no-ops on any panel whose vtable isn't CSWGuiFeatsCharGen, so the caller doesn't need a kind check. All field reads are SEH-guarded — safe to call on partially initialised panels.

## Declarations (in source order)

- L20 `void DumpStructureIfNeeded(void* panel)`: Called from menus_focus.cpp's AnnouncePanelTitle on every panel-title announce.
