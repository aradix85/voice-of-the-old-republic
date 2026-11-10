# unified_action_menu.cpp (353 lines)

Source: `patches/Accessibility/unified_action_menu.cpp`

The unified action menu: one keyboard menu over the engine's target rows (Shift+1..3) and personal columns (Shift+4..7). Here: its categories and selection, arming and disarming, and the suspend while an engine panel sits on top.

## Declarations (in source order)

- L33 `bool TargetRowsLookHostileCreature(void* tam)`: Decide whether the target block's three rows are the hostile-creature Attacks / Force-Powers / Items layout (→ named categories) vs a door/placeable/trigger's per-object actions (→ announce by label).
- L52 `bool ActionMenuAutoPauseEnabled()`: Vanilla parity: the radial / personal action menus auto-pause the world only when the "Action Menu" auto-pause option is on (CClientOptions bit 0x8000) — off by default.
- L61 `void RebuildAndRespeak(const char* what, const char* emptyReason)`: Rebuild the categories against the live menus (the engine may have re-populated them meanwhile), keep the cursor on the same category and re-speak it, so the user hears they are back at the same position.
- L81 `int ClampInt(int v, int lo, int hi)`
- L95 `bool ForegroundPanelBlocks()`: True when a real engine panel / modal / dialog owns the foreground.
- L101 `int CountForCat(void* tam, void* mi, const Cat& c)`
- L107 `int& ShadowFor(const Cat& c)`
- L115 `void ApplySelection(void* tam, void* mi, const Cat& c, int idx)`: Stamp the engine's per-slot selection to `idx` so reads + dispatch land on the user's chosen entry (target: field1 via SelectActionInRow; personal: the +0x1bac selected-action-id via SelectVariant).
- L125 `void ReadLabel(void* tam, void* mi, const Cat& c, int idx, char* out, size_t n)`: Read the label of the selected entry.
- L135 `uint32_t ReadActionId(void* tam, void* mi, const Cat& c, int idx)`
- L141 `bool Dispatch(void* tam, void* mi, const Cat& c)`
- L150 `const char* CategoryName(const Cat& c)`: Localised category name, or nullptr for an unnamed category (target rows on a non-creature target — door / placeable / trigger — whose rows carry per-object actions like Security / Bash and are announced by label).
- L182 `void FormatEmptyColumn(int col, char* out, size_t n)`: " : empty", or "Column N is empty" for a column without a name.
- L195 `void BuildCategoryList(void* tam, void* mi)`
- L222 `int LocateCat(CatKind kind, int slot)`
- L229 `int FirstPopulatedTargetRow(void* tam)`
- L238 `bool DetectCreature(void* tam, uint32_t handle)`: Combined creature signal: trust the clean vtable downcast for near targets, fall back to action-content for far / unresolvable ones.
- L247 `uint32_t ResolveNarratedServerHandle()`: Server-side handle of the currently narrated target, or 0 if none / map pin / stale.
- L256 `void Arm()`
- L274 `int PersonalSelection(int col)`
- L279 `int TargetSelection(int row)`
- L284 `bool IsActive()`
- L286 `bool IsSuspended()`
- L294 `void ReannounceCurrent()`: Re-speak the current category against the live menus.
- L307 `void SetForegroundBlocked(bool blocked)`: Panel-stack suspend / resume.
- L331 `void ForceDisarm(const char* reason)`
