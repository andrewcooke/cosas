
#include <iostream>

#include "cosas/node.h"


Constant::Constant(const int16_t v) : value(v) {};

int16_t Constant::next(int32_t /* phi */) {
  return value;
}

Constant zero = Constant(0);


Sequence::Sequence(std::initializer_list<int16_t> vs)
  : values(std::move(std::make_unique<std::list<int16_t>>(vs))) {};

int16_t Sequence::next(int32_t /* phi */) {
  if (!values->empty()) {
    const int16_t val = values->front();
    values->pop_front();
    return val;
  } else {
    return 0;
  }
}


Latch::Latch() : source(&zero) {};

void Latch::set_source(RelSource* s) {
  source = s;
}

int16_t Latch::next(int32_t phi) {
  if (source && ! on) {
    auto s = SetOnInScope(this);
    previous = source->next(phi);
  }
  return previous;
}

SetOnInScope::SetOnInScope(Latch* l) : latch(l) {
  latch->on = true;
}

SetOnInScope::~SetOnInScope() {
  latch->on = false;
}
