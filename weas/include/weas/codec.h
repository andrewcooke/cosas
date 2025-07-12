
#ifndef WEAS_CODEC_H
#define WEAS_CODEC_H

#include <sys/types.h>

#include "cosas/maths.h"


// codec as in DAC and ADC

class Codec {

public:
  Codec(const Codec&) = delete;
  Codec& operator=(const Codec&) = delete;
  static Codec& get();
  void adc_callback();  // public for static function
  enum Knob {Main, X, Y};
  enum Switch {Down, Middle, Up};

private:
  static constexpr uint PLS_IN = 2;
  static constexpr uint PLS_OUT = 8;
  static constexpr uint DAC_SCK = 18;
  static constexpr uint DAC_TX = 19;
  static constexpr uint DAC_CS = 21;
  static constexpr uint CV_OUT = 22;
  static constexpr uint MUX_OUT = 24;
  static constexpr uint ADC_IN = 26;
  static constexpr uint MUX_IN = 28;
  static constexpr float SAMPLE_FREQ = 44100;
  static constexpr uint OVERSAMPLE_BITS = 1;  // can't exceed 4(?) as we accumulate 12 bits into 16 bits
  Codec();
  [[nodiscard]] uint16_t read_adc(uint off, uint oversample_bits, bool fix) const;
  template<typename T> static void roll(T (&arr)[2], uint16_t val);
  template<typename T, unsigned int N> static void roll(T (&arr)[2][N], uint index, uint16_t val);
  uint32_t count = 0;
  uint8_t adc_dma, dac_dma;
  uint16_t adc_buffer[2][4 * (1 << OVERSAMPLE_BITS)] = {};  // [phase][over]
  uint16_t spi_buffer[2][2] = {};  // [phase][l/r]
  volatile int16_t cv[2] = {};
  volatile int16_t adc[2] = {0x800, 0x800};
  volatile bool pulse[2][2] = {};  // [p/c][l/r]
  volatile Switch switch_[2] = {};
  volatile uint16_t knobs[2][4] = {};

};

template<typename T> inline void Codec::roll(T (&arr)[2], uint16_t val) {
  arr[1] = arr[0];
  arr[0] = val;
}

template<typename T, unsigned int N> inline void Codec::roll(T (&arr)[2][N], uint index, uint16_t val) {
  arr[1][index] = arr[0][index];
  arr[0][index] = val;
}

inline uint16_t Codec::read_adc(const uint off, const uint oversample_bits, const bool fix) const {
  uint16_t adc = 0;
  for (int ov = 0; ov < 1 << oversample_bits; ov++) adc += adc_buffer[cpu_phase][off + 4 * ov];
  adc = adc >> oversample_bits;
  if (fix) adc = fix_dnl(adc);
  return adc;
}

#endif
