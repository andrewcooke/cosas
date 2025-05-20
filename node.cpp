
#include "node.h"


Latch::Latch() : source(nullptr) {};

void Latch::set_source(Source* s) {
  source = s;
}

int16_t Latch::next(int32_t tick, int32_t phi) {
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

int16_t Constant::next(int32_t tick, int32_t phi) {
  return value;
}


Sequence::Sequence(std::initializer_list<int16_t> vs)
  : values(std::move(std::make_unique<std::list<int16_t>>(vs))) {};

int16_t Sequence::next(int32_t tick, int32_t phi) {
  if (values->size() > 0) {
    int16_t val = values->front();
    values->pop_front();
    return val;
  } else {
    return 0;
  }
}
