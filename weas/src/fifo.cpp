
#include "pico/multicore.h"

#include "weas/fifo.h"
#include "weas/debug.h"


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

// TODO - in memory?
void FIFO::handle_ctrl_change(CtrlEvent event) {
  if (!stalled.Load() || event.ctrl == CtrlEvent::Switch) push(event);
}

// void FIFO::handle_connected_change(uint8_t socket_in, bool changed) {
//   uint32_t packed = CONNECTED | ((socket_in & 0x7) << 1) | changed;
//   push(packed);
// }

void FIFO::push(CtrlEvent event) {
  static uint write = 0, queued = 0;
  // if (!(write & DUMP_MASK)) Debug::log("write", queued, "/", write);
  write++;
  if (stalled.Load()) {
    queue.add(event);
    queued++;
    return;
  } else {
    Debug::log("direct");
    while (!queue.empty()) {
      CtrlEvent pending = queue.pop();
      if (!multicore_fifo_push_timeout_us(pending.pack(), TIMEOUT_US)) {
        queue.add(pending);
        queue.add(event);
        queued++;
        return;
      }
    }
    if (!multicore_fifo_push_timeout_us(event.pack(), TIMEOUT_US)) {
      queue.add(event);
      queued++;
    }
  }
}

// TODO - in memory?
void FIFO::core1_marshaller() {
  auto& fifo = get();
  uint read = 0;
  while (true) {
    // if (!(read & DUMP_MASK)) Debug::log("read", read);
    read++;
    uint32_t packed = multicore_fifo_pop_blocking();  // blocking wait
    fifo.ctrl_changes->handle_ctrl_change(CtrlEvent::unpack(packed));
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
  Debug::log("stalled -----");
  fifo.stalled = true;
}

 Stalled::~Stalled() {
  fifo.stalled = false;
  Debug::log("unstalled ------");
}


