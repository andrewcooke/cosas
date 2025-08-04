
#ifndef WEAS_FIFO_H
#define WEAS_FIFO_H

#include "pico/multicore.h"

#include "codec.h"


// responsible for launching code on core1 and connecting fifos
// that send knob change events.

// this is the basis for a UI using the knobs.


typedef std::function<void(uint8_t knob, uint16_t now, uint16_t prev)> event_handler;


class FIFO final : public KnobChanges {

public:
  FIFO(const FIFO&) = delete;
  FIFO& operator=(const FIFO&) = delete;
  static FIFO& get();
  void set_knob_changes(KnobChanges* k) {knob_changes = k;};
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) override;
  void start(Codec& cc);
  // void stop();

private:
  FIFO();
  KnobChanges* knob_changes;
  static void core1_marshaller();
};


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

FIFO::FIFO() : knob_changes(nullptr) {
  // TODO - pico specific things to set up FIFO?
}

// TODO - in memory?
void FIFO::handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) {
  uint32_t packed = knob << 28 | prev << 16 | now;
  multicore_fifo_push_timeout_us(packed, 0);
}

// TODO - in memory?
void FIFO::core1_marshaller() {
  while (true) {
    static auto& fifo = FIFO::get();
    uint32_t packed = multicore_fifo_pop_blocking();
    uint8_t knob = packed >> 28;
    uint16_t prev = (packed >> 16 & 0xfff);
    uint16_t now = packed & 0xfff;
    fifo.knob_changes->handle_knob_change(knob, now, prev);
  }
}

void FIFO::start(Codec& cc) {
  multicore_launch_core1(core1_marshaller);
  cc.set_knob_changes(this);
  cc.select_knob_changes(true);
}


#endif
