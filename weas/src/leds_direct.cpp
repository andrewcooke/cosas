
#include <algorithm>

#include "weas/leds_direct.h"


// todo - left to right
void LEDsDirect::display12bits(const uint16_t v) {
  uint16_t vv = v & 0x0fff;
  for (uint i = leds.N; i > 0; i--) {
    leds.set(i - 1, static_cast<uint8_t>((vv & 0x03) << 6));
    vv >>= 2;
  }
}

void LEDsDirect::display12bits(const int16_t v) {
  display12bits(static_cast<uint16_t>(v + 0x8000));
}

void LEDsDirect::column10levels(const uint c, uint8_t v) {
  column10levels(true, c, v);
}

void LEDsDirect::column3levels(const uint c, const uint8_t v) {
  for (uint i = 0; i < leds.N / 2; i++) {
    leds.set(4 - 2 * i + (c & 0x1), i == v);
  }
}

void LEDsDirect::display7levels(const uint8_t n) {
  for (uint i = 0; i < leds.N; i++) leds.set(i, i < n);
}

void LEDsDirect::columns12bits(uint16_t v) {
  v = v & 0x0fff;
  uint8_t v1 = v / 409;
  uint8_t v2 = (v % 409) / 41;
  column10levels(0, v1);
  column10levels(1, v2);
}

void LEDsDirect::columns12bits(int16_t v) {
  if (v >= 0) {
    columns11bits(true, v);
  } else {
    columns11bits(false, -v);
  }
}

void LEDsDirect::columns11bits(bool up, uint16_t v) {
  v = v & 0x07ff;
  uint8_t v1 = v / 204;
  uint8_t v2 = (v % 204) / 21;
  column10levels(up, 0, v1);
  column10levels(up, 1, v2);
}

void LEDsDirect::column10levels(const bool up, const uint c, uint8_t v) {
  v = std::min(static_cast<uint8_t>(9), v);
  for (uint i = 0; i < leds.N / 2; i++) {
    uint led = up ? 4 + (c & 0x1) - 2 * i : (c & 0x1) + 2 * i;
    leds.set(led, static_cast<uint8_t>(std::min(static_cast<uint8_t>(3), v) << 6));
    v = v > 3 ? v - 3 : 0;
  }
}

void LEDsDirect::display13levels(uint8_t v) {
  v = std::min(static_cast<uint8_t>(12), v);
  const bool inv = v > 6;
  if (inv) v -= 6;
  for (uint i = 0; i < leds.N; i++) {
    leds.set(i, (i == v) != inv);
  }
}

void LEDsDirect::sq4(const uint n, const uint8_t v) {
  for (uint i = 0; i < 4; i++) {
    leds.set(i + (n & 1) * 2, v);
  }
}

void LEDsDirect::v2(const uint n, const uint8_t v) {
  leds.set(n & 3, v);
  leds.set((n & 3) + 2, v);
}

void LEDsDirect::v3(const uint n, uint8_t v) {
  leds.set(n & 1, v);
  leds.set((n & 1) + 2, v);
  leds.set((n & 1) + 4, v);
}

void LEDsDirect::h2(uint n, uint8_t v) {
  n = n % 3;
  leds.set(n * 2, v);
  leds.set(n * 2 + 1, v);
}

auto LEDsDirect::display7bits(const int16_t v) -> void {
  const bool neg = v < 0;
  uint16_t vv = abs(v) & 0x3f;
  for (uint i = 0; i < leds.N; i++) {
    if (vv & 1) leds.set(i, neg ? 0x20 : 0xffu);
    else leds.set(i, 0x00u);
    vv >>= 1;
  }
}
