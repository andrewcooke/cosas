
#include "pico/multicore.h"

#include "weas/fifo.h"
#include "weas/debug.h"


FIFO& FIFO::get() {
  static FIFO instance;
  return instance;
}

// TODO - in memory?
void FIFO::handle_ctrl_change(CtrlEvent event) {
  push(event);
}

// void FIFO::handle_connected_change(uint8_t socket_in, bool changed) {
//   uint32_t packed = CONNECTED | ((socket_in & 0x7) << 1) | changed;
//   push(packed);
// }

void FIFO::push(CtrlEvent event) {
  static uint write = 0, queued = 0;
  write++;
  if (stalled.Load()) {
    queue.add(event);
    queued++;
    return;
  } else {
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
    read++;
    uint32_t packed = multicore_fifo_pop_blocking();  // blocking wait
    fifo.ctrl_changes->handle_ctrl_change(CtrlEvent::unpack(packed));
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


