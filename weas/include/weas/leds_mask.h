
#ifndef WEAS_LEDS_MASK_H
#define WEAS_LEDS_MASK_H

#include <array>

#include "pico/types.h"
#include "weas/leds.h"


// a compact description of leds that we can build on for animation


class LEDsMask {
public:

  LEDsMask();;
  void show(uint32_t mask);
  void show(uint32_t mask, uint32_t extra, uint32_t count);
  static uint32_t reverse(uint32_t mask);
  static uint32_t vinterp(uint off, uint32_t a, uint32_t b);
  static uint32_t hinterp(uint off, uint32_t a, uint32_t b);
  static uint32_t ring(float normalized, bool highlight);
  static uint32_t square(uint bottom, uint amplitude);
  static uint32_t vbar(uint right, uint amplitude);

private:

  static constexpr uint BITS = 5;  // can't be more than 5 for uint32_t
  static constexpr uint BITS_MASK = (1 << BITS) - 1;
  static constexpr uint32_t FULL_MASK = (1 << (LEDs::N * BITS)) - 1;
  static constexpr uint32_t SIDE_MASK = BITS_MASK | (BITS_MASK << (2 * BITS)) | (BITS_MASK << 4 * BITS);
  static constexpr uint CYCLE_BITS = 4;
  static constexpr uint N_CYCLE = 1 << CYCLE_BITS;
  std::array<float, N_CYCLE> pulse;
  LEDs& leds = LEDs::get();
  static constexpr uint ring_order[LEDs::N] = {4, 2, 0, 1, 3, 5};
  static uint32_t overwrite(uint32_t mask, float centre, float width, uint amplitude);

};


#endif
