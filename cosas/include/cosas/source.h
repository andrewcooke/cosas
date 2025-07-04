
#ifndef COSA_SOURCE_H
#define COSA_SOURCE_H

#include <cstdint>


class AbsSource {
public:
  virtual ~AbsSource() = default;
  [[nodiscard]] virtual int16_t next(int32_t tick) const = 0;
};


class RelSource {
public:
  virtual ~RelSource() = default;
  // typically delta is 1
  // cannot be const because oscillator tracks absolute time
  [[nodiscard]] virtual int16_t next(int32_t delta, int32_t phi) = 0;
};


#endif
