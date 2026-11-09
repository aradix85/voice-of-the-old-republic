# engine_scriptvar.h (44 lines)

Source: `patches/Accessibility/engine_scriptvar.h`

In-save named-variable persistence on the player creature's CSWSScriptVarTable (CSWSObject +0x110). The reusable storage primitive for any mod state that must survive save/reload WITHOUT a sidecar file: the engine serializes this table into the .sav via CSWSObject::SaveObjectState and repopulates it on load, so mutating it in memory is enough — no save hook. Full RE reference + addresses: docs/llm-docs/persistence-scriptvartable.md.

All calls reach the SERVER-side player creature (GetPlayerServerCreature) and operate on creature+0x110. Every call is SEH-guarded and degrades to a safe default (false / fallback / "") when no creature is loaded (main menu, pre-chargen) or any accessor faults — callers never need to gate on game state themselves.

Variable identity: keyed by (name AND type) together. An int "FOO" and a string "FOO" are independent entries; this mirrors the engine.

## Declarations (in source order)

- L27 `int GetPlayerVarInt(const char* name, int fallback = 0)`: Read a named int var.
- L31 `bool SetPlayerVarInt(const char* name, int value)`: Write a named int var.
- L37 `bool GetPlayerVarString(const char* name, char* outBuf, size_t bufSize)`: Read a named string var into outBuf (always NUL-terminated on entry).
- L41 `bool SetPlayerVarString(const char* name, const char* value)`: Write a named string var (copied into the table-owned CExoString).
