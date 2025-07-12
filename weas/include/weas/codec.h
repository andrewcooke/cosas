
#ifndef WEAS_CODEC_H
#define WEAS_CODEC_H

#include <sys/types.h>


// codec as in DAC and ADC

class Codec {

private:
  static constexpr uint DAC_SCK = 18;
  static constexpr uint DAC_TX = 19;
  static constexpr uint DAC_CS = 21;
  static constexpr uint MUX_OUT = 24;
  static constexpr uint ADC_IN = 26;
  static constexpr uint MUX_IN = 28;
  static constexpr float SAMPLE_FREQ = 44100;
  static constexpr uint OVERSAMPLE = 2;
  Codec();
  uint32_t count = 0;
  uint8_t adc_dma, spi_dma;
  uint16_t adc_buffer[2][4 * OVERSAMPLE], spi_buffer[2][2];  // [phase][l/r/over]
  uint cpu_phase = 1, dma_phase = 0;  // alternating indices into buffers
  volatile int16_t cv[2] = {0, 0};
  volatile int16_t adc[2] = {0x800, 0x800};
  volatile bool pulse[2][2];  // [p/c][l/r]

public:
  Codec(const Codec&) = delete;
  Codec& operator=(const Codec&) = delete;
  static Codec& get();
  void adc_callback();
};


#endif
