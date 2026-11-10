# hotkeys.h (241 lines)

Source: `patches/Accessibility/hotkeys.h`

Centralised hotkey registry. Single source of truth for every mod-added binding. Per-tick pollers query Pressed(Action) for rising edge + modifier match + KOTOR foreground. The keybind configurator (menus_keybinds) rebinds through SetUserBinding.

We don't piggyback on the engine's [Keymapping] because it's bare DIK→Action with no modifier combos and no AltGr semantics.

## Declarations (in source order)

- L18 `enum ModifierBit`: Required + forbidden masks let one VK host multiple actions (e.g.
- L27 `enum class Action`: One per logical gesture.
- L159 `struct Binding`: vk + optional altVk for layout portability (same physical key reports different VKs on US QWERTY vs German QWERTZ).
- L167 `void BeginTick()`: Call at start/end of every per-frame dispatch.
- L168 `void EndTick()`
- L172 `bool Pressed(Action a)`: True iff binding VK (or altVk) is down this tick + was up last tick + modifiers match + KOTOR is foreground.
- L176 `bool Held(Action a)`: Pure held-state query — no edge, no foreground gate.
- L181 `void Consume(Action a)`: Claim the current tick's rising edge so other consumers don't see it.
- L187 `void ClaimRisingEdge(Action a)`: Pre-claim the NEXT rising edge.
- L190 `bool ShiftHeld()`: Direct OS-level modifier reads — no edge, no foreground gate.
- L191 `bool CtrlHeld()`
- L195 `uint32_t CurrentModifiers()`: Current OS modifier mask (kMod* bits) — the same read BeginTick uses for edge matching.
- L203 `bool ModifiedComboOwns(int vk)`: True iff some registered binding on `vk` (primary or alt key) REQUIRES at least one modifier and currently matches the live keyboard (its key is down, every required modifier is held, no forbidden modifier is held).
- L205 `bool IsForegroundGame()`
- L209 `Binding Get(Action a)`: IsUserRebindable returns false for diagnostic probes (excluded from the rebind UI).
- L210 `void Set(Action a, Binding b)`
- L211 `bool IsUserRebindable(Action a)`
- L215 `void SetUserBinding(Action a, Binding b)`: Apply a user rebind: update the live binding AND persist it to acc_settings.ini so it survives a relaunch.
- L219 `void ResetUserBindings()`: Reset every user-rebindable action to its factory default and rewrite the persisted overrides to match.
- L227 `Action FindConflict(Action self, int vk, uint32_t mods)`: Conflict scan for the configurator.
- L231 `const char* Name(Action a)`: Describe(a) goes into a rotating static buffer so multiple calls in one log line stay valid (acclog::FmtPtr convention).
- L232 `const char* Describe(Action a)`
- L238 `const char* VkLabel(int vk)`: Spoken/printable label for a bare VK ("V", "F5", "Enter", ...).
