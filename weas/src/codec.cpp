
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

Codec::Codec() {
  run_mode = Running;
  adc_run(false);
  adc_select_input(0);

  use_norm_probe = false;
  for (uint i = 0; i < N_SOCKET_IN; i++) connected[i] = false;

  gpio_init(NORMALISATION_PROBE);
  gpio_set_dir(NORMALISATION_PROBE, GPIO_OUT);
  gpio_put(NORMALISATION_PROBE, false);

  adc_init();

  for (uint lr = 0; lr < N_CHANNELS; lr++) adc_gpio_init(AUDIO_IN + lr);
  for (uint mux = 0; mux < N_MUX; mux++) adc_gpio_init(MUX_IN + mux);

  for (uint mux = 0; mux < 2; mux++) {
    gpio_init(MUX_LOGIC + mux);
    gpio_set_dir(MUX_LOGIC + mux, GPIO_OUT);
  }

  for (uint lr = 0; lr < N_CHANNELS; lr++) {
    gpio_init(PULSE_RAW_OUT + lr);
    gpio_set_dir(PULSE_RAW_OUT + lr, GPIO_OUT);
    gpio_put(PULSE_RAW_OUT + lr, true); // raw high (output low)
  }

  for (uint lr = 0; lr < N_CHANNELS; lr++) {
    gpio_init(PULSE_INPUT + lr);
    gpio_set_dir(PULSE_INPUT + lr, GPIO_IN);
    gpio_pull_up(PULSE_INPUT); // needs pullup to activate transistor on inputs
  }

  spi_init(spi0, 15625000);
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(DAC_SCK, GPIO_FUNC_SPI);
  gpio_set_function(DAC_TX, GPIO_FUNC_SPI);
  gpio_set_function(DAC_CS, GPIO_FUNC_SPI);

  i2c_init(i2c0, 100 * 1000);
  gpio_set_function(EEPROM_SDA, GPIO_FUNC_I2C);
  gpio_set_function(EEPROM_SCL, GPIO_FUNC_I2C);

  for (uint lr = 0; lr < N_CHANNELS; lr++) gpio_set_function(CV_OUT + lr, GPIO_FUNC_PWM);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_wrap(&config, 0x7ff); // 11-bit PWM
  // CV_A and CV_B share the same PWM slice, which means that they share a PWM config
  // they have separate 'gpio_level's (output compare unit) though, so they can have different PWM on-times
  pwm_init(pwm_gpio_to_slice_num(CV_OUT), &config, true); // no need to set again
  for (uint lr = 0; lr < N_CHANNELS; lr++) pwm_set_gpio_level(CV_OUT + lr, scale_cv_out(0x800));
}
