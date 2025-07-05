
#ifndef COSAS_QUEUE_H
#define COSAS_QUEUE_H

#include <array>
#include <cstdint>
#include <functional>


// circular; empty when read_from == write_to
class Buffer {
public:
  Buffer(std::function<int16_t(int32_t)>& cb, std::function<void(uint32_t)>& slp);
  int16_t next();
  void run();
private:
  std::function<int16_t(int32_t)>& callback;
  std::function<void(uint32_t)>& sleep_us;
  static constexpr int16_t EMPTY = 0;
  // signed to make arithmetic for space simpler
  volatile int read_from;
  int write_to;
  static constexpr size_t BUFFER_LEN = 5;
  std::array<int16_t, BUFFER_LEN> buffer;
};


#endif

