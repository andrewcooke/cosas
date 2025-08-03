
#ifndef WEAS_LEDS_TIMER_H
#define WEAS_LEDS_TIMER_H

#include "weas/codec.h"
#include "weas/leds_mask.h"

#include <memory>
#include <pico/time.h>
#include <pico/types.h>


// this uses a timer on the core on which it is (first) used


class LEDsTimer {

public:
  static constexpr uint TIMER_HZ = 100;
  static LEDsTimer& get(Codec& codec);
  void set_speed(uint speed);
  void show(uint32_t mask);
  void show(uint32_t mask, uint32_t extra);
  // void loop(int delta, std::vector<std::tuple<uint32_t, uint32_t>> mex);
  static bool trampoline(repeating_timer_t *rt);

private:
  LEDsTimer(Codec& codec);
  void lazy_start_on_local_core();
  alarm_pool_t* alarm_pool = nullptr;
  Codec& codec;
  uint speed = 12;
  uint32_t mask;
  uint32_t extra;
  repeating_timer_t out = {};
  std::unique_ptr<LEDsMask> leds_mask = std::make_unique<LEDsMask>();
  void render();
};


#endif
