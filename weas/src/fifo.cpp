
#include "pico/multicore.h"

#include "weas/fifo.h"
#include "weas/debug.h"


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

// TODO - in memory?
void FIFO::handle_ctrl_change(uint8_t ctrl, uint16_t now, uint16_t prev) {
  if (!stalled || ctrl == Codec::Switch) {
    uint32_t packed = CTRL | ((ctrl & 0x3) << 24 | (prev & 0xfff) << 12 | (now & 0xfff));
    push(packed);
  }
}

void FIFO::handle_connected_change(uint8_t socket_in, bool changed) {
  uint32_t packed = CONNECTED | ((socket_in & 0x7) << 1) | changed;
  push(packed);
}

void FIFO::push(uint32_t msg) {
  static uint write = 0, dropped_write = 0;
  // if (!(write & DUMP_MASK)) Debug::log("write", dropped_write, "/", write);
  write++;
  while (!overflow.empty()) {
    uint32_t pending = overflow.front();
    if (multicore_fifo_push_timeout_us(pending, TIMEOUT_US)) {
      overflow.pop();
    } else {
      dropped_write++;
      overflow.push(OVERFLOW | msg);
      return;
    }
  }
  if (!multicore_fifo_push_timeout_us(msg, TIMEOUT_US)) {
    dropped_write++;
    overflow.push(OVERFLOW | msg);
  }
}

// TODO - in memory?
void FIFO::core1_marshaller() {
  auto& fifo = get();
  uint read = 0, read_dropped = 0;
  while (true) {
    // if (!(read & DUMP_MASK)) Debug::log("read", read_dropped, "/", read);
    read++;
    uint32_t packed = multicore_fifo_pop_blocking();  // blocking wait
    switch (packed & TAG_MASK) {
    case CTRL: {
      uint8_t ctrl = (packed >> 24) & 0x3;
      if ((packed & OVERFLOW) && (ctrl != Codec::Switch)) {
        read_dropped++;
        break;  // discard to clear backlog
      }
      uint16_t now = packed & 0xfff;
      uint16_t prev = (packed >> 12) & 0xfff;
      fifo.ctrl_changes->handle_ctrl_change(ctrl, now, prev);
      break;
    }
    case CONNECTED: {
      bool connected = packed & 0x1;
      uint8_t socket_in = (packed >> 1) & 0x7;
      fifo.handle_connected_change(socket_in, connected);
    }
    default: break;
    }
  }
}

void FIFO::start(Codec& cc) {
  multicore_launch_core1(core1_marshaller);
  cc.set_ctrl_changes(this);
  cc.select_ctrl_changes(true);
}


 Stalled::Stalled(FIFO &fifo) : fifo(fifo) {
  fifo.stalled = true;
}

 Stalled::~Stalled() {
  fifo.stalled = false;
}


