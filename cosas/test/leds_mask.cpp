
#include "doctest/doctest.h"

#include "cosas/leds_mask.h"


class LEDsMask final : public BaseLEDsMask {
public:
  LEDsMask(uint8_t bits) : BaseLEDsMask(bits) {};
  uint32_t mask = 0;
  void show(uint32_t m) override {mask = m;}
};


TEST_CASE("LEDsMask, reverse") {
  auto leds_mask = LEDsMask(4);
  CHECK(leds_mask.reverse(0x123456) == 0x654321);
}

TEST_CASE("LEDsMask, vinterp") {
  auto leds_mask = LEDsMask(4);
  CHECK(leds_mask.vinterp(0, 0x123456, 0xabcdef) == 0x123456);
  CHECK(leds_mask.vinterp(1, 0x123456, 0xabcdef) == 0x3456ab);
  CHECK(leds_mask.vinterp(2, 0x123456, 0xabcdef) == 0x56abcd);
  CHECK(leds_mask.vinterp(3, 0x123456, 0xabcdef) == 0xabcdef);
}

TEST_CASE("LEDsMask, rotate") {
  auto leds_mask = LEDsMask(4);
  CHECK(leds_mask.rotate(0x123456, 1) == 0x234561);
  CHECK(leds_mask.rotate(0x123456, 2) == 0x345612);
  CHECK(leds_mask.rotate(0x123456, 3) == 0x456123);
  CHECK(leds_mask.rotate(0x123456, 4) == 0x561234);
  CHECK(leds_mask.rotate(0x123456, 5) == 0x612345);
  CHECK(leds_mask.rotate(0x123456, 6) == 0x123456);
}

TEST_CASE("LEDsMask, rot2dot") {
  auto leds_mask_4 = LEDsMask(4);
  CHECK(leds_mask_4.rot2dot(1, leds_mask_4.BITS_MASK >> 3, leds_mask_4.BITS_MASK >> 1) == 0x717177);
  auto leds_mask_5 = LEDsMask(5);
  CHECK(leds_mask_5.rot2dot(1, leds_mask_5.BITS_MASK >> 3, leds_mask_5.BITS_MASK >> 1) == 0x1e378d3f);
}
