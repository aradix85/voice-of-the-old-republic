# engine_scriptvar.cpp (148 lines)

Source: `patches/Accessibility/engine_scriptvar.cpp`

The player's script variables: the engine accessors behind ReadPlayerVar and WritePlayerVar, and the settle wait. The model and the save / reload proof are in docs/llm-docs/persistence-scriptvartable.md.

## Declarations (in source order)

- L62 `void ExoInit(ExoStr* s, const char* cstr)`
- L68 `void ExoFree(ExoStr* s)`
- L77 `void* PlayerVarTable()`: creature+0x100, or nullptr when no server-side player creature is loaded.
- L85 `bool GetPlayerVarString(const char* name, char* outBuf, size_t bufSize)`: Reads into outBuf, always NUL-terminated; true for a non-empty value.
- L107 `bool SetPlayerVarString(const char* name, const char* value)`
- L124 `std::string ReadPlayerVar(const std::string& name)`
- L129 `bool WritePlayerVar(const std::string& name, const std::string& value)`
- L133 `void PlayerVarsSettle::Reset()`
- L138 `bool PlayerVarsSettle::Tick()`
