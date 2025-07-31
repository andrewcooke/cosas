
#ifndef WEAS_LEDS_MASK_H
#define WEAS_LEDS_MASK_H


#include "pico/types.h"
#include "weas/leds.h"


class LEDsMask {

public:

  LEDsMask() = default;
  void show(uint32_t s);
  uint32_t ring(float normalized, bool highlight);

private:

  LEDs& leds = LEDs::get();
  uint ring_order[LEDs::N] = {4, 2, 0, 1, 3, 5};
  uint32_t overwrite(uint32_t m, float n, float w, uint a);

};


#endif
