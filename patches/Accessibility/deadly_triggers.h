// Trigger areas whose script kills whoever steps in, and the gate through
// them.
//
// The pylons before the Elder compound on the Unknown World (South Beach,
// unk_m41ab) arc lightning between them: eleven script triggers that answer a
// step with 998 damage. A sighted player sees the arcs and walks through the
// one opening, where a twelfth trigger starts the hologram that takes the
// inner ring away. Nothing says any of it in text, and a walk to the compound
// door runs straight through an arc.
//
// Such triggers are known by tag. While one stands, its area is a standing
// zone of turnback_zones.h, so the mod's own path stays out of it; when its
// script destroys the trigger, the zone goes with it.

#pragma once

#include "walkmesh_geometry.h"  // Vector

namespace acc::guidance::deadly {

// Makes the standing zones of turnback_zones.h those of the deadly triggers
// that stand in `area` now (and forgets the zones of any other area). Call
// before a walk is judged or a path computed.
void Refresh(void* area);

// The middle of the gate through the deadly triggers the last Refresh found:
// the trigger whose script opens the way. False when none stands.
bool Gate(Vector& out);

}  // namespace acc::guidance::deadly
