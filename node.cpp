
#include "node.h"


Latch::Latch() : source(nullptr) {};

void Latch::set_source(Source* s) {
  source = s;
}

int16_t Latch::next(int64_t tick, int32_t phi) {
  if (! on) {
    On(*this);
    previous = source->next(tick, phi);
  }
  return previous;
}

On::On(Latch& l) : latch(l) {
  latch.on = true;
}

On::~On() {
  latch.on = false;
}


Constant::Constant(int16_t v) : value(v) {};

int16_t Constant::next(int64_t tick, int32_t phi) {
  return value;
}
