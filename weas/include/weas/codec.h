
#ifndef WEAS_CODEC_H
#define WEAS_CODEC_H

#include <functional>
#include <utility>
#include <sys/types.h>

#include "cosas/maths.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/spi.h"


constexpr uint SAMPLE_48K = 48000;
constexpr uint SAMPLE_44_1K = 44100;

enum Knob {Main, X, Y};
enum Switch {Down, Middle, Up};


// codec as in DAC and ADC

template<uint SAMPLE_FREQ, uint OVER_BITS> class Codec {

public:

  Codec(const Codec&) = delete;
  Codec& operator=(const Codec&) = delete;

  static Codec& get() {
    static Codec instance;
    return instance;
  }

  void set_callback(std::function<void()> f) {
    callback = std::move(f);
  }

  void isr() {
    isr_pre();
    callback();
    isr_post();
  }

  [[nodiscard]] uint16_t get_adc(const uint lr) const {
    return adc[lr & 0x1];
  }

  [[nodiscard]] uint16_t get_cv(const uint lr) const {
    return cv[lr & 0x1];
  }

  [[nodiscard]] bool get_pulse(const uint lr) const {
    return pulse[0][lr & 0x1];
  }

  [[nodiscard]] bool chg_pulse(const uint lr) const {
    return pulse[0][lr & 0x1] != pulse[1][lr & 0x1];
  }

  [[nodiscard]] uint16_t get_knob(const Knob k) const {
    return knobs[0][k];
  }

  [[nodiscard]] bool chg_knob(const Knob k) const {
    return knobs[0][k] != knobs[1][k];
  }

  int32_t get_count() const {
    return count;
  }

  void start_irq(bool block) {
    irq_set_enabled(DMA_IRQ_0, true);
    irq_set_exclusive_handler(DMA_IRQ_0, [](){get().isr();});
    adc_run(true);
    while (block) sleep_ms(1000);
  }

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

  static constexpr uint ADC_OFF = 0;
  static constexpr uint CV_OFF = 2;
  static constexpr uint MUX_OFF = 3;

  Codec() {

    adc_run(false);
    adc_select_input(0);
    adc_init();

    for (uint lr = 0; lr < 2; lr++) {
      adc_gpio_init(ADC_IN + lr);
      gpio_init(PLS_OUT + lr);
      gpio_set_dir(PLS_OUT + lr, GPIO_OUT);
      gpio_put(PLS_OUT + lr, true);
      gpio_init(PLS_IN + lr);
      gpio_set_dir(PLS_IN+ lr, GPIO_IN);
      gpio_pull_up(PLS_IN + lr);
    }

    for (uint mux = 0; mux < 2; mux++) {
      adc_gpio_init(MUX_IN + mux);
      gpio_init(MUX_OUT + mux);
      gpio_set_dir(MUX_OUT + mux, GPIO_OUT);
    }

    spi_init(spi0, 15625000);  // why this baudrate?  max is 20MHz
    spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(DAC_SCK, GPIO_FUNC_SPI);
    gpio_set_function(DAC_TX, GPIO_FUNC_SPI);
    gpio_set_function(DAC_CS, GPIO_FUNC_SPI);

    for (uint lr = 0; lr < 2; lr++) gpio_set_function(CV_OUT + lr, GPIO_FUNC_PWM);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 2047); // why 11 bit?
    for (uint lr = 0; lr < 2; lr++) {
      pwm_init(pwm_gpio_to_slice_num(CV_OUT + lr), &config, true);
      pwm_set_gpio_level(CV_OUT + lr, 1024);
    }

    adc_set_round_robin(0b1111);  // audio and MUX
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(48000000 / (4 * (1 << OVER_BITS) * SAMPLE_FREQ) - 1);
    adc_dma = dma_claim_unused_channel(true);
    dac_dma = dma_claim_unused_channel(true);
    dma_channel_config adc_dmacfg = dma_channel_get_default_config(adc_dma);
    dma_channel_config dac_dmacfg = dma_channel_get_default_config(dac_dma);
    channel_config_set_transfer_data_size(&adc_dmacfg, DMA_SIZE_16);
    channel_config_set_read_increment(&adc_dmacfg, false);
    channel_config_set_write_increment(&adc_dmacfg, true);
    channel_config_set_dreq(&adc_dmacfg, DREQ_ADC);
    dma_channel_configure(adc_dma, &adc_dmacfg, adc_buffer[count & 0x1], &adc_hw->fifo, 4 << OVER_BITS, true);
    channel_config_set_transfer_data_size(&dac_dmacfg, DMA_SIZE_16);
    channel_config_set_dreq(&dac_dmacfg, DREQ_SPI0_TX);

    dma_channel_set_irq0_enabled(adc_dma, true);
  }

  void isr_pre() {
    static volatile int32_t knobs_smooth[4] = {0, 0, 0, 0};
    static volatile int32_t cv_smooth[2] = {0, 0};
    const uint cpu_phase = count & 0x1;

    const uint cv_idx = count & 0x1;
    cv_smooth[cv_idx] = (15 * (cv_smooth[cv_idx]) + 16 * read_adc(cpu_phase, CV_OFF, OVER_BITS, true)) >> 4;
    cv[cv_idx] = static_cast<int16_t>(2048 - cv_smooth[cv_idx]);

    for (uint lr = 0; lr < 2; lr++) {
      adc[lr] = -(read_adc(cpu_phase, ADC_OFF + lr, OVER_BITS, true) - 0x1000);
      roll(pulse, lr, !gpio_get(PLS_IN));
    }

    const uint knob_idx = count & 0x3;
    knobs_smooth[knob_idx] = (127 * (knobs_smooth[knob_idx]) + 16 * read_adc(cpu_phase, MUX_OFF, 0, false)) >> 7;
    roll(knobs, knob_idx, static_cast<uint16_t>(knobs_smooth[knob_idx] >> 4));
    if (knob_idx == 3) roll(switch_, static_cast<Switch>((knobs[0][knob_idx] > 1000) + (knobs[0][knob_idx] > 3000)));
  }

  void isr_post() {
    count++;
    const uint dma_phase = count & 0x1;
    adc_select_input(0);  // is this needed here?
    for (uint mux = 0; mux < 2; mux++) gpio_put(MUX_OUT + mux, count & (0x1 << mux));
    dma_channel_set_write_addr(adc_dma, adc_buffer[dma_phase], true);
    dma_channel_set_read_addr(dac_dma, dac_buffer[dma_phase], true);

    dma_hw->ints0 = 0x1 << adc_dma;  // reset adc interrupt flag (we have handled it)
  }

  [[nodiscard]] inline uint16_t read_adc(const uint cpu_phase, const uint off, const uint oversample_bits, const bool fix) const {
    uint16_t adc = 0;
    for (int ov = 0; ov < 1 << oversample_bits; ov++) adc += adc_buffer[cpu_phase][off + 4 * ov];
    adc = adc >> oversample_bits;
    if (fix) adc = fix_dnl(adc);
    return adc;
  }

  // TODO - do non-volatile imply an error?

  template<typename T> inline static void roll(T (&arr)[2], T val) {
    arr[1] = arr[0];
    arr[0] = val;
  }

  template<typename T> inline static void roll(volatile T (&arr)[2], T val) {
    arr[1] = arr[0];
    arr[0] = val;
  }

  template<typename T, unsigned int N> inline static void roll(T (&arr)[2][N], uint index, T val) {
    arr[1][index] = arr[0][index];
    arr[0][index] = val;
  }

  template<typename T, unsigned int N> inline static void roll(volatile T (&arr)[2][N], uint index, T val) {
    arr[1][index] = arr[0][index];
    arr[0][index] = val;
  }

  std::function<void()> callback = [](){};
  uint32_t count = 0;
  uint8_t adc_dma, dac_dma;
  uint16_t adc_buffer[2][4 * (1 << OVER_BITS)] = {};  // [phase][idx/over]
  uint16_t dac_buffer[2][2] = {};  // [phase][l/r]
  volatile int16_t cv[2] = {};
  volatile int16_t adc[2] = {0x800, 0x800};
  volatile bool pulse[2][2] = {};  // [prev/cur][l/r]
  volatile Switch switch_[2] = {};
  volatile uint16_t knobs[2][4] = {};

};

#endif
