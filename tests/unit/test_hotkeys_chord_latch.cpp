// Pairing a hotkey combo with the engine's buffered press of its key
// (hotkeys_chord_latch).

#include "doctest.h"
#include "hotkeys_chord_latch.h"

using acc::hotkeys::ChordLatch;

TEST_CASE("The engine's press after the chord, modifier released, is the chord's, once") {
    ChordLatch latch;
    latch.ChordDown('L', 1000);
    CHECK(latch.TakeEnginePress('L', 1120));
    CHECK_FALSE(latch.TakeEnginePress('L', 1130));
}

TEST_CASE("A press swallowed under the modifier leaves nothing to wait for") {
    // The engine's press first, then the poll.
    ChordLatch engineFirst;
    engineFirst.EngineSwallowed('L', 1000);
    engineFirst.ChordDown('L', 1010);
    CHECK_FALSE(engineFirst.TakeEnginePress('L', 1200));

    // The poll first, then the engine's press with the modifier still held.
    ChordLatch pollFirst;
    pollFirst.ChordDown('L', 1000);
    pollFirst.EngineSwallowed('L', 1010);
    CHECK_FALSE(pollFirst.TakeEnginePress('L', 1200));
}

TEST_CASE("Each press of the combo is paired on its own") {
    ChordLatch latch;
    latch.EngineSwallowed('L', 1000);
    latch.ChordDown('L', 1010);
    latch.ChordDown('L', 1300);
    CHECK(latch.TakeEnginePress('L', 1400));
}

TEST_CASE("Another key, or a press half a second later, stays the engine's") {
    ChordLatch latch;
    latch.ChordDown('L', 1000);
    CHECK_FALSE(latch.TakeEnginePress('M', 1100));
    CHECK_FALSE(latch.TakeEnginePress(0, 1100));
    CHECK_FALSE(latch.TakeEnginePress('L', 1500));

    ChordLatch unbound;
    unbound.ChordDown(0, 1000);
    CHECK_FALSE(unbound.TakeEnginePress(0, 1001));
}
