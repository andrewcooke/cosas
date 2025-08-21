
#ifndef WEAS_LEDS_BUFFER_H
#define WEAS_LEDS_BUFFER_H


#include <pico/time.h>
#include <pico/types.h>

#include "cosas/leds_buffer.h"

#include "weas/leds_mask.h"


class LEDsBuffer final : public BaseLEDsBuffer {

public:
  static constexpr size_t TIMER_MS = 20;
  static LEDsBuffer& get();

private:
  explicit LEDsBuffer() : BaseLEDsBuffer(std::make_unique<LEDsMask>(5)) {};
  void lazy_start_on_local_core() override;
  static bool trampoline(repeating_timer_t *rt);
  alarm_pool_t* alarm_pool = nullptr;
  repeating_timer_t out = {};
};


#endif
