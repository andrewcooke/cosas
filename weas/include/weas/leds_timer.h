
#ifndef WEAS_LEDS_TIMER_H
#define WEAS_LEDS_TIMER_H

#include "weas/codec.h"
#include "weas/leds_mask.h"

#include <memory>
#include <pico/time.h>
#include <pico/types.h>


class LEDsTimer {

public:
  static constexpr uint TIMER_HZ = 100;
  static LEDsTimer& get(Codec& codec);
  void show(uint32_t mask);
  void show(uint32_t mask, uint32_t extra);
  static bool trampoline(repeating_timer_t *rt);

private:
  LEDsTimer(Codec& codec);
  alarm_pool_t* alarm_pool = nullptr;
  Codec& codec;
  int32_t mask;
  int32_t extra;
  repeating_timer_t out = {};
  std::unique_ptr<LEDsMask> leds_mask = std::make_unique<LEDsMask>();
  void render();
};


#endif
