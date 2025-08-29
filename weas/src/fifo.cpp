
#include "pico/multicore.h"

#include "weas/fifo.h"
#include "weas/debug.h"


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

// TODO - in memory?
void FIFO::handle_ctrl_change(CtrlEvent event) {
  if (!stalled || event.ctrl == CtrlEvent::Switch) push(event);
}

// void FIFO::handle_connected_change(uint8_t socket_in, bool changed) {
//   uint32_t packed = CONNECTED | ((socket_in & 0x7) << 1) | changed;
//   push(packed);
// }

void FIFO::push(CtrlEvent event) {
  uint32_t msg = event.pack();
  static uint write = 0, dropped_write = 0;
  // if (!(write & DUMP_MASK)) Debug::log("write", dropped_write, "/", write);
  write++;
  while (!overflow.empty()) {
    uint32_t pending = overflow.front();
    if (multicore_fifo_push_timeout_us(pending, TIMEOUT_US)) {
      overflow.pop();
    } else {
      dropped_write++;
      overflow.push(Header::Overflow | msg);
      return;
    }
  }
  if (!multicore_fifo_push_timeout_us(msg, TIMEOUT_US)) {
    dropped_write++;
    overflow.push(Header::Overflow | msg);
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
    CtrlEvent event = CtrlEvent::unpack(packed);
    if (packed & Overflow && event.ctrl != CtrlEvent::Switch) {
      read_dropped++;
    } else {
      fifo.ctrl_changes->handle_ctrl_change(event);
    }
    // case CONNECTED: {
    //   bool connected = packed & 0x1;
    //   uint8_t socket_in = (packed >> 1) & 0x7;
    //   fifo.handle_connected_change(socket_in, connected);
    // }
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


