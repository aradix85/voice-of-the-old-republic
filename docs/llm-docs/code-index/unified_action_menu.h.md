# unified_action_menu.h (108 lines)

Source: `patches/Accessibility/unified_action_menu.h`

Unified in-world action menu — one navigable menu over BOTH the engine's target-action rows (CSWGuiTargetActionMenu, 3 categories) and the player's personal action bar (CSWGuiMainInterface, up to 4 populated categories).

Replaces the former three components (radial_menu / target_action_menu / actionbar_menu): blind users don't need the visual target-vs-personal split the engine renders, so we present one menu whose categories carry the distinction by name.

Navigation (identical for every category): Left / Right        — previous / next category (clamp at the ends) Up / Down           — previous / next entry within the category (clamp) Home / End          — first / last entry of the current category Ctrl+Home / Ctrl+End— first / last category Shift + any arrow   — speak the selected entry's full description, no move Enter               — fire the selected entry; Esc — cancel

Category order, left → right: the target block first (so Shift+Enter lands on Attacks for a hostile creature), then the personal block. Empty categories are skipped.

Entry points: ArmFromRadial  — Shift+Enter (the picker has already opened the radial). OpenTarget     — Shift+1/2/3 (jump to a target category). OpenPersonal   — Shift+4/5/6/7 (jump to a personal column). Bare 1..7 are NOT handled here — the engine fires those instantly; only the Shift gestures open this menu.

Engine read/primitive layers live in engine_radial (target) and engine_actionbar (personal); this module only orchestrates + speaks.

## Declarations (in source order)

- L43 `bool OpenTarget(int row)`: Open on a specific TARGET category (Shift+1/2/3, row 0..2).
- L54 `bool OpenPersonal(int col)`: Open on a specific PERSONAL column (Shift+4/5/6/7, col 0..3).
- L61 `bool FirePersonal(int col)`: Fire a personal column's currently-selected entry outright, no menu — what bare 8 and 9 do for KOTOR 2's combat-behaviour and forms columns.
- L69 `bool ArmFromRadial(const char* name, uint32_t targetHandle)`: Arm from the Shift+Enter radial path.
- L71 `bool IsActive()`
- L78 `void ReannounceCurrent()`: Re-speak the current category against the live menus, re-locating the cursor on the same slot.
- L85 `bool IsSuspended()`: True while armed but suspended under a blocking engine panel (a MessageBox or a hotkey-opened sub-screen).
- L91 `void SetForegroundBlocked(bool blocked)`: Per-tick panel-stack notification, driven by input_poll_router.
- L97 `bool HandleInputEvent(int code, int value)`: Routed by input_poll_router's Win32 poll.
- L99 `void ForceDisarm(const char* reason)`
- L104 `int PersonalSelection(int col)`: Bare-key announce support: the user's last-selected index for a personal column / target row, kept in lock-step with the engine's per-slot selection so a bare 1..7 press announces what the engine actually fires.
- L105 `int TargetSelection(int row)`
