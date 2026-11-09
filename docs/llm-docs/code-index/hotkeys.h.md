# hotkeys.h (267 lines)

Source: `patches/Accessibility/hotkeys.h`

Centralised hotkey registry. Single source of truth for every mod-added binding. Per-tick pollers query Pressed(Action) for rising edge + modifier match + KOTOR foreground. Bindings are runtime-mutable via Set() for a future rebind UI.

We don't piggyback on the engine's [Keymapping] because it's bare DIK→Action with no modifier combos and no AltGr semantics.

## Declarations (in source order)

- L18 `enum ModifierBit`: Required + forbidden masks let one VK host multiple actions (e.g.
- L27 `enum class Action`: One per logical gesture.
- L165 `struct Binding`: vk + optional altVk for layout portability (same physical key reports different VKs on US QWERTY vs German QWERTZ).
- L173 `void BeginTick()`: Call at start/end of every per-frame dispatch.
- L174 `void EndTick()`
- L178 `bool Pressed(Action a)`: True iff binding VK (or altVk) is down this tick + was up last tick + modifiers match + KOTOR is foreground.
- L182 `bool Held(Action a)`: Pure held-state query — no edge, no foreground gate.
- L187 `void Consume(Action a)`: Claim the current tick's rising edge so other consumers don't see it.
- L193 `void ClaimRisingEdge(Action a)`: Pre-claim the NEXT rising edge.
- L207 `void PadPress(Action a)`: Fire `a` from a source that is not the keyboard — currently the KOTOR 2 gamepad, whose buttons never set a VK.
- L210 `bool ShiftHeld()`: Direct OS-level modifier reads — no edge, no foreground gate.
- L211 `bool CtrlHeld()`
- L212 `bool AltHeld()`
- L213 `bool AltGrHeld()`
- L217 `uint32_t CurrentModifiers()`: Current OS modifier mask (kMod* bits) — the same read BeginTick uses for edge matching.
- L225 `bool ModifiedComboOwns(int vk)`: True iff some registered binding on `vk` (primary or alt key) REQUIRES at least one modifier and currently matches the live keyboard (its key is down, every required modifier is held, no forbidden modifier is held).
- L227 `bool IsForegroundGame()`
- L231 `Binding Get(Action a)`: IsUserRebindable returns false for diagnostic probes (excluded from the rebind UI).
- L232 `void Set(Action a, Binding b)`
- L233 `bool IsUserRebindable(Action a)`
- L237 `Binding GetDefault(Action a)`: Factory default for an action (the binding before any user override).
- L241 `void SetUserBinding(Action a, Binding b)`: Apply a user rebind: update the live binding AND persist it to acc_settings.ini so it survives a relaunch.
- L245 `void ResetUserBindings()`: Reset every user-rebindable action to its factory default and rewrite the persisted overrides to match.
- L253 `Action FindConflict(Action self, int vk, uint32_t mods)`: Conflict scan for the configurator.
- L257 `const char* Name(Action a)`: Describe(a) goes into a rotating static buffer so multiple calls in one log line stay valid (acclog::FmtPtr convention).
- L258 `const char* Describe(Action a)`
- L264 `const char* VkLabel(int vk)`: Spoken/printable label for a bare VK ("V", "F5", "Enter", ...).
