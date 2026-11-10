// engine_keymap_codes: InputIndices translation and the [Keymapping] line.

#include "doctest.h"
#include "engine_keymap.h"
#include "engine_keymap_codes.h"

using namespace acc::engine_keymap;

TEST_CASE("InputIndices letters and digits map to their virtual keys") {
    CHECK(InputIndexToVk(0x33) == 'A');
    CHECK(InputIndexToVk(0x4c) == 'Z');
    CHECK(InputIndexToVk(0x4d) == '1');
    CHECK(InputIndexToVk(0x56) == '0');
    CHECK(InputIndexToVk(0x57) == 0x20);  // VK_SPACE
    CHECK(InputIndexToVk(0x1e) == 0x09);  // VK_TAB
    CHECK(InputIndexToVk(0x27) == 0x70);  // VK_F1
    CHECK(InputIndexToVk(0) == 0);
    CHECK(InputIndexToVk(0x6b) == 0);     // a gap in the enum
}

TEST_CASE("Scancodes follow the physical key, not the alphabet") {
    CHECK(InputIndexToScancode(0x33) == 0x1E);                    // A
    CHECK(InputIndexToScancode(0x33 + ('W' - 'A')) == 0x11);      // W
    CHECK(InputIndexToScancode(0x33 + ('Y' - 'A')) == 0x15);      // Y
    CHECK(InputIndexToScancode(0x33 + ('Z' - 'A')) == 0x2C);      // Z
    CHECK(InputIndexToScancode(0x4d) == 0x02);                    // 1
    CHECK(InputIndexToScancode(0x56) == 0x0B);                    // 0
    CHECK(InputIndexToScancode(0x27 + 3) == 0x3E);                // F4
    CHECK(InputIndexToScancode(89) == 0);                         // Caps Lock: never injected
}

TEST_CASE("A keymapping line gives action, slot and key") {
    KeymapEntry e{};
    REQUIRE(ParseKeymapEntry("Action280A=73", e));
    CHECK(e.actionId == 280);
    CHECK(e.slot == 'A');
    CHECK(e.inputIndex == 73);

    REQUIRE(ParseKeymapEntry("action284b=53", e));
    CHECK(e.actionId == 284);
    CHECK(e.slot == 'B');

    REQUIRE(ParseKeymapEntry("Action207=72", e));
    CHECK(e.actionId == 207);
    CHECK(e.slot == 0);
    CHECK(e.inputIndex == 72);

    REQUIRE(ParseKeymapEntry("Other=5", e));  // still a bound key
    CHECK(e.actionId == 0);
    CHECK(e.inputIndex == 5);

    CHECK_FALSE(ParseKeymapEntry("no equals sign", e));
}
