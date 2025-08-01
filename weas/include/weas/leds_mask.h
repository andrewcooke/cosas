
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
  uint32_t reverse(uint32_t mask);
  uint32_t ring(float normalized, bool highlight);
  uint32_t top_square(uint amplitude);

private:

  static constexpr uint BITS = 5;  // can't be more than 5 for uint32_t
  static constexpr uint BITS_MASK = (1 << BITS) - 1;
  static constexpr uint32_t FULL_MASK = (1 << (LEDs::N * BITS)) - 1;
  static constexpr uint CYCLE_BITS = 4;
  static constexpr uint N_CYCLE = 1 << CYCLE_BITS;
  std::array<float, N_CYCLE> scale;
  LEDs& leds = LEDs::get();
  uint ring_order[LEDs::N] = {4, 2, 0, 1, 3, 5};
  uint32_t overwrite(uint32_t mask, float centre, float width, uint amplitude);

};


#endif
