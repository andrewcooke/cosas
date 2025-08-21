
#ifndef WEAS_LEDS_MASK_H
#define WEAS_LEDS_MASK_H


#include "cosas/leds_mask.h"


class LEDsMask final : public BaseLEDsMask {

public:
  explicit LEDsMask(uint8_t bits) : BaseLEDsMask(bits) {};
  void show(uint32_t mask) override;

};

#endif

