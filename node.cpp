
#include <iostream>

#include "node.h"


Constant::Constant(const int16_t v) : value(v) {};

int16_t Constant::next(int32_t /* tick */, int32_t /* phi */) const {
  return value;
}

Constant zero = Constant(0);


Sequence::Sequence(std::initializer_list<int16_t> vs)
  : values(std::move(std::make_unique<std::list<int16_t>>(vs))) {};

int16_t Sequence::next(int32_t /* tick */, int32_t /* phi */) const {
  if (values->size() > 0) {
    int16_t val = values->front();
    values->pop_front();
    return val;
  } else {
    return 0;
  }
}


Latch::Latch() : source(&zero) {};

void Latch::set_source(const Source* s) const {
  source = s;
}

int16_t Latch::next(int32_t tick, int32_t phi) const {
  if (! on) {
    auto s = SetOnInScope(this);
    previous = source->next(tick, phi);
  }
  return previous;
}

SetOnInScope::SetOnInScope(const Latch* l) : latch(l) {
  latch->on = true;
}

SetOnInScope::~SetOnInScope() {
  latch->on = false;
}
