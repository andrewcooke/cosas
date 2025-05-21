
#include <iostream>

#include "node.h"


Constant::Constant(int16_t v) : value(v) {};

int16_t Constant::next(int32_t tick, int32_t phi) {
  return value;
}

Constant zero = Constant(0);


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


Latch::Latch() : source(zero) {};

void Latch::set_source(Source& s) {
  // std::cout << "setting source " << &s << std::endl;
  source = s;
  // std::cout << "next " << next(0, 0) << std::endl;
}

int16_t Latch::next(int32_t tick, int32_t phi) {
  // std::cout << "calling source " << &source << " " << on << std::endl;
  if (! on) {
    On(*this);
    previous = source.next(tick, phi);
  }
  return previous;
}

On::On(Latch& l) : latch(l) {
  latch.on = true;
}

On::~On() {
  latch.on = false;
}
