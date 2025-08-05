
#include "weas/fifo.h"


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

FIFO::FIFO() {
  // TODO - pico specific things to set up FIFO?
}

// TODO - in memory?
void FIFO::handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) {
  uint32_t packed = KNOB | ((knob & 0x3) << 24 | (prev & 0xfff) << 12 | (now & 0xfff));
  push(packed);
  // multicore_fifo_push_timeout_us(packed, 0);
}

void FIFO::handle_connected_change(uint8_t socket_in, bool changed) {
  uint32_t packed = CONNECTED | ((socket_in & 0x7) << 1) | changed;
  push(packed);
  // multicore_fifo_push_timeout_us(packed, 0);
}

void FIFO::push(uint32_t msg) {
  while (!overflow.empty()) {
    uint32_t pending = overflow.front();
    if (multicore_fifo_push_timeout_us(pending, 0)) {
      overflow.pop();
    } else {
      overflow.push(OVERFLOW | msg);
      return;
    }
  }
  if (!multicore_fifo_push_timeout_us(msg, 0)) overflow.push(OVERFLOW | msg);
}

// TODO - in memory?
void FIFO::core1_marshaller() {
  auto& fifo = get();
  while (true) {
    uint32_t packed = multicore_fifo_pop_blocking();
    switch (packed & TAG_MASK) {
    case KNOB: {
      uint8_t knob = (packed >> 24) & 0x3;
      if ((packed & OVERFLOW) && (knob != Codec::Switch)) break;  // discard to clear backlog
      uint16_t prev = (packed >> 12 & 0xfff);
      uint16_t now = packed & 0xfff;
      fifo.knob_changes->handle_knob_change(knob, now, prev);
      break;
    }
    case CONNECTED: {
      uint8_t socket_in = (packed >> 1) & 0x7;
      bool connected = packed & 0x1;
      fifo.handle_connected_change(socket_in, connected);
    }
    default: break;
    }

  }
}

void FIFO::start(Codec& cc) {
  multicore_launch_core1(core1_marshaller);
  cc.set_knob_changes(this);
  cc.select_knob_changes(true);
}


