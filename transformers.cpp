
#include <cstdint>

#include "constants.h"
#include "maths.h"
#include "transformers.h"


Gain::Gain(Node& nd, const Amplitude& amp)
  : Transformer(nd), amplitude(amp) {};

uint16_t Gain::next(int64_t tick, int32_t phi) {
  return amplitude.scale(node.next(tick, phi));
}


