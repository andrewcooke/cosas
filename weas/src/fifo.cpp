
#include "pico/multicore.h"

#include "weas/fifo.h"
#include "weas/debug.h"


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

FIFO::FIFO() {
  // TODO - pico specific things to set up FIFO?
}

// TODO - in memory?
void FIFO::handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) {
  if (knob != Codec::Switch) {
    now = filter[Now][knob].next_or(now, SAME);
    prev = filter[Prev][knob].next(prev);
  }
  if (now != SAME && abs(static_cast<int>(now) - prev) > FILTER_CUTOFF) {
    // Debug::log("change", static_cast<int>(knob), now, prev);
    uint32_t packed = KNOB | ((knob & 0x3) << 24 | (prev & 0xfff) << 12 | (now & 0xfff));
    push(packed);
  }
}

void FIFO::handle_connected_change(uint8_t socket_in, bool changed) {
  uint32_t packed = CONNECTED | ((socket_in & 0x7) << 1) | changed;
  push(packed);
}

void FIFO::push(uint32_t msg) {
  while (!overflow.empty()) {
    uint32_t pending = overflow.front();
    if (multicore_fifo_push_timeout_us(pending, TIMEOUT_US)) {
      overflow.pop();
    } else {
      overflow.push(OVERFLOW | msg);
      Debug::log("overflow (1)", ++overflow_count, overflow.size());
      return;
    }
  }
  if (!multicore_fifo_push_timeout_us(msg, TIMEOUT_US)) {
    overflow.push(OVERFLOW | msg);
    Debug::log("overflow (2)", ++overflow_count, overflow.size());
  }
}

// TODO - in memory?
void FIFO::core1_marshaller() {
  auto& fifo = get();
  while (true) {
    uint32_t packed = multicore_fifo_pop_blocking();  // blocking wait
    switch (packed & TAG_MASK) {
    case KNOB: {
      uint8_t knob = (packed >> 24) & 0x3;
      if ((packed & OVERFLOW) && (knob != Codec::Switch)) break;  // discard to clear backlog
      // uint16_t prev = (packed >> 12) & 0xfff;
      uint16_t now = packed & 0xfff;
      Debug::log("unpacked", static_cast<int>(knob), now);
      // fifo.knob_changes->handle_knob_change(knob, now, prev);
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


