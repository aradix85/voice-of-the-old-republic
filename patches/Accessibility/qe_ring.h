// Q/E all around — mod setting "Q and E: everything around you".
//
// The engine's Q/E (SelectNearestObject) only cycles what the camera shows,
// so a player who cannot see the screen has to turn the camera to find
// things with it, while , / . reach everything regardless of the camera.
// With this setting on, the mod takes the Q/E press (input_pipeline) and
// cycles every object around the player itself:
//
//   * candidates: creatures (not the party; out of combat everyone alive, in
//     combat only hostiles), usable doors (not cosmetic ones), usable
//     placeables, items on the ground and area transitions, within the
//     engine's own target range, not behind a wall;
//   * order: clockwise by clock position from straight ahead, nearer first
//     at the same angle; E steps clockwise, Q back;
//   * the landing is spoken and stamped exactly like an explicit engine Q/E
//     (passive_narrate::NarrateExplicit), and the engine's own target and
//     action lists are pointed at it, so Enter, R and 1..3 act on it.
//
// Off by default: with it off the engine's Q/E is untouched.

#pragma once

namespace acc::qe_ring {

bool Enabled();

// A Q (-1) or E (+1) press in the world. True when the mod handled it (the
// caller swallows the engine event); false leaves it to the engine (setting
// off, not in the world, a screen or dialogue up).
bool HandlePress(int direction);

}  // namespace acc::qe_ring
