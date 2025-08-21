
#ifndef COSAS_LEDS_BUFFER_H
#define COSAS_LEDS_BUFFER_H


#include "cosas/leds_mask.h"

#include <memory>
#include <queue>


class BaseLEDsBuffer {

public:
  static constexpr size_t INTERP_BITS = 2;
  static constexpr size_t INTERP_N = 1 << INTERP_BITS;
  static BaseLEDsBuffer& get();
  // TODO - not clear we need force (since n=0 does the same)
  void queue(uint32_t mask, bool force, bool interp, size_t n);
  uint32_t get_mask();
  const std::unique_ptr<BaseLEDsMask> leds_mask;

protected:
  explicit BaseLEDsBuffer(std::unique_ptr<BaseLEDsMask> leds_mask) : leds_mask(std::move(leds_mask)) {};
  ~BaseLEDsBuffer() = default;
  virtual void lazy_start_on_local_core() {};
  void render();

private:
  uint32_t mask = 0;
  std::queue<std::tuple<uint32_t, bool>> buffer;
};


#endif
