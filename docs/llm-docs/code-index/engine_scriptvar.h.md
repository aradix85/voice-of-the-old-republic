# engine_scriptvar.h (47 lines)

Source: `patches/Accessibility/engine_scriptvar.h`

In-save named-variable persistence on the player creature's CSWSScriptVarTable (CSWSObject +0x110). The reusable storage primitive for any mod state that must survive save/reload WITHOUT a sidecar file: the engine serializes this table into the .sav via CSWSObject::SaveObjectState and repopulates it on load, so mutating it in memory is enough — no save hook. Full RE reference + addresses: docs/llm-docs/persistence-scriptvartable.md.

All calls reach the SERVER-side player creature (GetPlayerServerCreature) and operate on creature+0x110. Every call is SEH-guarded and degrades to a safe default (false / fallback / "") when no creature is loaded (main menu, pre-chargen) or any accessor faults — callers never need to gate on game state themselves.

Variable identity: keyed by (name AND type) together. An int "FOO" and a string "FOO" are independent entries; this mirrors the engine.

## Declarations (in source order)

- L25 `std::string ReadPlayerVar(const std::string& name)`: The value of a named string variable; empty when it is absent or empty, when there is no creature and on a fault.
- L29 `bool WritePlayerVar(const std::string& name, const std::string& value)`: Writes a named string variable (copied into the table's own CExoString).
- L36 `class PlayerVarsSettle`: The wait before the variables can be trusted: the player creature has to have been the same object for about a second.
