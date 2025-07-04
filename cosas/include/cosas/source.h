
#ifndef COSA_SOURCE_H
#define COSA_SOURCE_H

#include <cstdint>


class AbsSource {
public:
  virtual ~AbsSource() = default;
  [[nodiscard]] virtual int16_t next(int32_t tick, int32_t phi) const = 0;
};


class RelSource {
public:
  virtual ~RelSource() = default;
  // typically delta is 1
  [[nodiscard]] virtual int16_t next(int32_t delta, int32_t phi) const = 0;
};


size_t tick2idx(int32_t tick, int32_t phi);
int32_t hz2tick(float hz);
uint32_t hz2freq(float hz);


#endif
