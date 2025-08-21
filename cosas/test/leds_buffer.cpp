
#include "doctest/doctest.h"

#include "cosas/leds_buffer.h"


class LEDsMask final : public BaseLEDsMask {
public:
  explicit LEDsMask(uint8_t bits) : BaseLEDsMask(bits) {};
  uint32_t mask = 0;
  void show(uint32_t m) override {mask = m;}
};

class LEDsBuffer final : public BaseLEDsBuffer {
public:
  LEDsBuffer() : BaseLEDsBuffer(std::make_unique<LEDsMask>(4)) {};
  uint32_t next() {
    render();
    return static_cast<LEDsMask*>(leds_mask.get())->mask;
  };
};


TEST_CASE("LEDsBuffer, simple") {
  auto leds_buffer = LEDsBuffer();
  leds_buffer.queue(0x123456, false, false, 0);
  CHECK(leds_buffer.next() == 0x123456);
}

TEST_CASE("LEDsBuffer, interp1") {
  auto leds_buffer = LEDsBuffer();
  leds_buffer.queue(0x000000, false, false, 0);
  leds_buffer.queue(0x123456, false, true, 0);
  CHECK(leds_buffer.next() == 0x000000);
  CHECK(leds_buffer.next() == 0x000111);
  CHECK(leds_buffer.next() == 0x011223);
  CHECK(leds_buffer.next() == 0x012334);
  CHECK(leds_buffer.next() == 0x123456);
}

TEST_CASE("LEDsBuffer, interp2a") {
  auto leds_buffer = LEDsBuffer();
  leds_buffer.queue(0xf0f0f0, false, false, 0);
  leds_buffer.queue(0x0f0f0f, false, true, 0);
  CHECK(leds_buffer.next() == 0xf0f0f0);
  CHECK(leds_buffer.next() == 0xb3b3b3);
  CHECK(leds_buffer.next() == 0x777777);
  CHECK(leds_buffer.next() == 0x3b3b3b);
  CHECK(leds_buffer.next() == 0x0f0f0f);
}

TEST_CASE("LEDsBuffer, interp2b") {
  auto leds_buffer = LEDsBuffer();
  leds_buffer.queue(0xf0f0f0, false, false, 1);
  leds_buffer.queue(0x0f0f0f, false, true, 0);
  CHECK(leds_buffer.next() == 0xf0f0f0);  // that extra "1"
  CHECK(leds_buffer.next() == 0xf0f0f0);
  CHECK(leds_buffer.next() == 0xb3b3b3);
  CHECK(leds_buffer.next() == 0x777777);
  CHECK(leds_buffer.next() == 0x3b3b3b);
  CHECK(leds_buffer.next() == 0x0f0f0f);
}

TEST_CASE("LEDsBuffer, interp3") {
  auto leds_buffer = LEDsBuffer();
  leds_buffer.queue(0xf0f0f0, false, false, 0);
  CHECK(leds_buffer.next() == 0xf0f0f0);
  leds_buffer.queue(0x0f0f0f, false, true, 0);
  CHECK(leds_buffer.next() == 0xf0f0f0);
  CHECK(leds_buffer.next() == 0xb3b3b3);
  CHECK(leds_buffer.next() == 0x777777);
  CHECK(leds_buffer.next() == 0x3b3b3b);
  CHECK(leds_buffer.next() == 0x0f0f0f);
}
