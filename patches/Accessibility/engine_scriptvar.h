// In-save named-variable persistence on the player creature's
// CSWSScriptVarTable (CSWSObject +0x110). The reusable storage primitive for
// any mod state that must survive save/reload WITHOUT a sidecar file: the
// engine serializes this table into the .sav via CSWSObject::SaveObjectState
// and repopulates it on load, so mutating it in memory is enough — no save
// hook. Full RE reference + addresses: docs/llm-docs/persistence-scriptvartable.md.
//
// All calls reach the SERVER-side player creature (GetPlayerServerCreature)
// and operate on creature+0x110. Every call is SEH-guarded and degrades to a
// safe default (false / fallback / "") when no creature is loaded (main menu,
// pre-chargen) or any accessor faults — callers never need to gate on game
// state themselves.
//
// Variable identity: keyed by (name AND type) together. An int "FOO" and a
// string "FOO" are independent entries; this mirrors the engine.

#pragma once

#include <string>

namespace acc::engine {

// The value of a named string variable; empty when it is absent or empty,
// when there is no creature and on a fault. Values up to 16 KB.
std::string ReadPlayerVar(const std::string& name);

// Writes a named string variable (copied into the table's own CExoString).
// False when no creature is loaded or an accessor faults.
bool WritePlayerVar(const std::string& name, const std::string& value);

// The wait before the variables can be trusted: the player creature has to
// have been the same object for about a second. On a load the creature is
// rebuilt and CSWSObject::LoadObjectState fills its table slightly later; a
// read before that gets an empty table and a write does not stick. One per
// reader: Reset on an area change, Tick once per frame until it is true.
class PlayerVarsSettle {
public:
    void Reset();
    bool Tick();

private:
    void* creature_ = nullptr;
    int   ticks_    = 0;
};

}  // namespace acc::engine
