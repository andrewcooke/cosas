
#ifndef COSAS_QUEUE_H
#define COSAS_QUEUE_H

#include <array>
#include <cstdint>
#include <functional>


// circular; empty when read_from == write_to
class AudioBuffer {

public:
  explicit AudioBuffer(std::function<int16_t(int32_t)>& cb);
  virtual ~AudioBuffer() = default;
  int16_t next();
  void run();

protected:
  // can be overridden on pico side
  virtual void sleep_us(uint32_t /* us */) {};
  virtual void log_delta(int /* d */) {};

private:
  std::function<int16_t(int32_t)>& callback;
  static constexpr int16_t EMPTY = 0;
  // signed to make arithmetic for space simpler
  volatile size_t read_from;
  size_t write_to;
  static constexpr size_t BUFFER_LEN = 5;
  std::array<int16_t, BUFFER_LEN> buffer;

};


#endif

