
import std;
using namespace std;

#include "node.h"


Latch::Latch() : source(nullptr) {};

void Latch::set_source(Source* s) {
  source = s;
}

uint16_t Latch::next(int64_t tick, int32_t phi) {
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


Constant::Constant(uint16_t v) : value(v) {};

uint16_t Constant::next(int64_t tick, int32_t phi) {
  return value;
}
