
import std;
using namespace std;

#include "node.h"


Latch::Latch(Source& s) : source(s) {};

uint16_t Latch::next(int64_t tick, int32_t phi) {
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
