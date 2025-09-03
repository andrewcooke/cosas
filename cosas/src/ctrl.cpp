
#include "cosas/ctrl.h"

#include "cosas/debug.h"
#include "cosas/common.h"


uint32_t CtrlEvent::pack() {
  return Header::Ctrl | ((ctrl & 0x3) << 24 | (prev & 0xfff) << 12 | (now & 0xfff));
}

CtrlEvent CtrlEvent::unpack(uint32_t packed) {
  return CtrlEvent(packed >> 24 & 0x3, packed & 0xfff, packed >> 12 & 0xfff);
}

bool CtrlEvent::operator==(const CtrlEvent &other) const {
  return other.ctrl == ctrl && other.now == now && other.prev == prev;
}

std::ostream& operator<<(std::ostream& os, const CtrlEvent& obj) {
  os << "CtrlEvent(" << obj.ctrl << "," << obj.now << "," << obj.prev << ")";
  return os;
}



void CtrlQueue::add(CtrlEvent event) {
  empty_ = false;
  if (event.ctrl == CtrlEvent::Switch) {
    queue[0] = event;
    queue[1] = CtrlEvent();
    queue[2] = CtrlEvent();
  } else if (event.ctrl != CtrlEvent::Dummy && queue[0].ctrl != CtrlEvent::Switch) {
    if (queue[event.ctrl].ctrl == CtrlEvent::Dummy) {
      queue[event.ctrl] = event;
    } else {
      queue[event.ctrl].now = event.now;
    }
  }
}

bool CtrlQueue::empty() {
  return empty_;
}

CtrlEvent CtrlQueue::pop() {
  empty_ = true;
  CtrlEvent found = CtrlEvent();
  for (size_t i = 0; i < N_KNOBS; i++) {
    size_t index = (offset + i) % N_KNOBS;
    if (queue[index].ctrl != CtrlEvent::Dummy) {
      if (found.ctrl == CtrlEvent::Dummy) {
        found = queue[index];
        queue[index] = CtrlEvent();
      } else {
        empty_ = false;
      }
    }
  }
  offset = (offset + 1) % N_KNOBS;
  return found;
}
