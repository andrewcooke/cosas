
#include "weas/codec.h"


uint32_t __not_in_flash_func(Codec::calc_adc_scale)() {
  const uint16_t adc_max = adc_correction(0xfff);
  return static_cast<uint32_t>((0xfff << 19) / adc_max);
}

uint16_t __not_in_flash_func(Codec::apply_adc_scale)(uint16_t v) {
  return (v * adc_scale) >> 19;
}

// pseudo-random bit for normalisation probe
uint32_t __not_in_flash_func(Codec::next_norm_probe)() {
  static uint32_t lcg_state = 1;
  lcg_state = 1664525 * lcg_state + 1013904223;
  return lcg_state >> 31;
}

uint16_t __not_in_flash_func(Codec::dac_value)(int16_t value, uint16_t dacChannel) {
  // cc had more complex logic here so i may be missing something
  return (dacChannel | 0x3000) | (0xfff & static_cast<uint16_t>(value + 0x800));
}

uint16_t __not_in_flash_func(Codec::scale_cv_out)(uint16_t v) {
  return v >> 1; // pwm is 11 bits to reduce ripple
}

void  __not_in_flash_func(Codec::stop)() {
  run_mode = ReqStop;
}
