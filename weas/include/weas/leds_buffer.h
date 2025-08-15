
#ifndef WEAS_LEDS_BUFFER_H
#define WEAS_LEDS_BUFFER_H

#include <memory>
#include <queue>

#include <pico/time.h>
#include <pico/types.h>


class LEDsBuffer {

public:
  static constexpr uint TIMER_MS = 100;
  static constexpr uint INTERP_BITS = 2;
  static constexpr uint INTERP_N = 1 << INTERP_BITS;
  static constexpr bool DISABLED = true;
  static LEDsBuffer& get();
  void queue(uint32_t mask, bool force, bool interp, uint n);
  static bool trampoline(repeating_timer_t *rt);
  uint32_t get_mask();

private:
  explicit LEDsBuffer() = default;
  void lazy_start_on_local_core();
  alarm_pool_t* alarm_pool = nullptr;
  uint32_t mask;
  std::queue<std::tuple<uint32_t, bool>> buffer;
  repeating_timer_t out = {};
  void render();
};


#endif
