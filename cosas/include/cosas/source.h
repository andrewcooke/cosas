
#ifndef COSA_SOURCE_H
#define COSA_SOURCE_H

#include <cstdint>


class Source {
public:
  // make sure we cleanly delete subclasses via a base pointer
  virtual ~Source() = default;
  // tick needs to be signed because phi can be negative and we need
  // to add the two.  also, next is perhaps not const because of Latch?
  // note - we can't merge phi into tick becuse it depends on the
  // source frequency (see oscillator.cpp)
  [[nodiscard]] virtual int16_t next(int32_t tick, int32_t phi) const = 0;
  [[nodiscard]] int16_t next(int32_t tick) const;
};


size_t tick2idx(int32_t tick, int32_t phi);
int32_t hz2tick(float hz);
uint32_t hz2freq(float hz);


#endif
