
#ifndef WEAS_FIFO_H
#define WEAS_FIFO_H

#include "pico/multicore.h"

#include "codec.h"


// responsible for launching code on core1 and connecting fifos
// that send knob change events.


typedef std::function<void(uint8_t knob, uint16_t now, uint16_t prev)> event_handler;


template <uint OVERSAMPLE_BITS, uint SAMPLE_FREQ>
class FIFO final {

public:
  FIFO(const FIFO&) = delete;
  FIFO& operator=(const FIFO&) = delete;
  static FIFO& get();
  void set_event_handler(event_handler f) {app_handler = f;};
  void start();
  void stop();

private:
  FIFO();
  event_handler app_handler = [](uint8_t, uint16_t, uint16_t) {};
  static void knob_changed_callback(typename Codec<OVERSAMPLE_BITS, SAMPLE_FREQ>::Knob knob, uint16_t now, uint16_t prev);
  static void core1_marshaller();
};


template <uint O, uint F> FIFO<O, F>& FIFO<O, F>::get() {
  static FIFO instance;
  return instance;
}

template <uint O, uint F> FIFO<O, F>::FIFO() {
  // TODO - pico specific things to set up FIFO
}

// TODO - in memory
template <uint O, uint F>
void FIFO<O, F>::knob_changed_callback(typename Codec<O, F>::Knob knob, uint16_t now, uint16_t prev) {
  uint32_t packed = static_cast<uint8_t>(knob) << 28 | prev << 16 | now;
  multicore_fifo_push_timeout_us(packed, 0);
}

// TODO - in memory
template <uint O, uint F> void FIFO<O, F>::core1_marshaller() {
  while (true) {
    static auto& fifo = FIFO<O, F>::get();
    uint32_t packed = multicore_fifo_pop_blocking();
    uint8_t knob = packed >> 28;
    uint16_t prev = (packed >> 16 & 0xfff);
    uint16_t now = packed & 0xfff;
    fifo.app_handler(knob, prev, now);
  }
}

template <uint O, uint F> void FIFO<O, F>::start() {
  auto& cc = Codec<O, F>::get();
  multicore_launch_core1(core1_marshaller);
  cc.set_knob_changed_cb(knob_changed_callback);
}

template <uint O, uint F> void FIFO<O, F>::stop() {
}


#endif
