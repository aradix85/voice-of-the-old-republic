# engine_scriptvar.cpp (157 lines)

Source: `patches/Accessibility/engine_scriptvar.cpp`

Promoted from the throwaway scriptvar_poc — the in-save named-variable primitive (player creature's CSWSScriptVarTable at +0x100). See the header and docs/llm-docs/persistence-scriptvartable.md for the model and the end-to-end save/reload proof.

(This line said +0x110 until 2026-07-31. That is precisely the Ghidra mislabelling the block above kScriptVarTableOffset exists to warn about, so the file was documenting the trap and repeating it in the same breath.)

## Declarations (in source order)

- L71 `void ExoInit(ExoStr* s, const char* cstr)`
- L77 `void ExoFree(ExoStr* s)`
- L82 `void* PlayerVarTable()`: creature+0x100, or nullptr when no server-side player creature is loaded.
- L91 `int GetPlayerVarInt(const char* name, int fallback)`
- L104 `bool SetPlayerVarInt(const char* name, int value)`
- L118 `bool GetPlayerVarString(const char* name, char* outBuf, size_t bufSize)`
- L140 `bool SetPlayerVarString(const char* name, const char* value)`
