
#ifndef COSAS_SOURCE_H
#define COSAS_SOURCE_H

#include <cstdint>


class AbsSource {
public:
  virtual ~AbsSource() = default;
  [[nodiscard]] virtual int16_t next(int32_t tick) const = 0;
};


class RelSource {
public:
  virtual ~RelSource() = default;
  // cannot be const because oscillator tracks absolute time
  [[nodiscard]] virtual int16_t next(int32_t phi) = 0;
};


#endif
