
#ifndef WEAS_FIFO_H
#define WEAS_FIFO_H


#include <queue>

#include "pico/multicore.h"

#include "codec.h"
#include "cosas/filter.h"


// responsible for launching code on core1 and connecting fifos
// that send knob change events.

// this is the basis for a UI using the knobs.


class FIFO final : public KnobChanges, public ConnectedChanges {

public:

  // topmost bits
  static constexpr uint32_t OVERFLOW = 0x1 << 31;
  static constexpr uint32_t KNOB = 0x0 << 30;
  static constexpr uint32_t CONNECTED = 0x1 << 30;
  static constexpr uint32_t TAG_MASK = KNOB | CONNECTED;
  // then quite a bit of space before the payload

  FIFO(const FIFO&) = delete;
  FIFO& operator=(const FIFO&) = delete;
  static FIFO& get();

  void set_knob_changes(KnobChanges* k) {knob_changes = k;};
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) override;
  void set_connected_changes(ConnectedChanges* c) {connected_changes = c;};
  void handle_connected_change(uint8_t socket_in, bool connected) override;
  void start(Codec& cc);
  // void stop();

private:

  FIFO();
  KnobChanges* knob_changes = nullptr;
  ConnectedChanges* connected_changes = nullptr;
  void push(uint32_t msg);
  static void core1_marshaller();
  std::queue<uint32_t> overflow;
  static constexpr uint16_t SAME = 4096;
  static constexpr uint8_t FILTER_SIZE = 2;
  MovingAverage<FILTER_SIZE> filter[Codec::N_WHEN][3] = {
    MovingAverage<FILTER_SIZE>(), MovingAverage<FILTER_SIZE>(),
    MovingAverage<FILTER_SIZE>(), MovingAverage<FILTER_SIZE>(),
    MovingAverage<FILTER_SIZE>(), MovingAverage<FILTER_SIZE>()
  };
};


#endif
