
#include "cosas/ctrl.h"


uint32_t CtrlEvent::pack() {
  return Header::Ctrl | ((ctrl & 0x3) << 24 | (prev & 0xfff) << 12 | (now & 0xfff));
}

CtrlEvent CtrlEvent::unpack(uint32_t packed) {
  return CtrlEvent(packed >> 24 & 0x3, packed & 0xfff, packed >> 12 & 0xfff);
}
