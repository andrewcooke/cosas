
#ifndef WEAS_CODEC_H
#define WEAS_CODEC_H


#include <functional>

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "weas.h"
#include "cosas/ui.h"


// the interface to the DAC and ADC


static constexpr uint CC_SAMPLE_44_1 = 44100;
static constexpr uint CC_SAMPLE_8 = 48000;


template <uint OVERSAMPLE_BITS, uint SAMPLE_FREQ>
class Codec final {

public:

  static constexpr uint OVERSAMPLES = 1 << OVERSAMPLE_BITS;

  enum Knob { Main, X, Y, Switch };
  enum SwitchPosition { Down, Middle, Up };
  static constexpr uint N_KNOBS = Switch + 1;
  enum SocketIn { Audio1, Audio2, CV1, CV2, Pulse1, Pulse2 };
  static constexpr uint N_SOCKET_IN = Pulse2 + 1;
  enum ADCBitFlag {  // we could include knobs too...?
    A1 = 1, A2 = 2, AllA = 3,
    C1 = 4, C2 = 8, AllC = 12,
    All = 15
  };
  enum When { Now, Prev };
  static constexpr uint N_WHEN = Prev + 1;

  Codec(const Codec&) = delete;
  Codec& operator=(const Codec&) = delete;

  void start();
  void stop(); // TODO - example of use?

  static Codec& get() {
    static Codec cc;
    return cc;
  }

  [[nodiscard]] int32_t get_count() const { return count; }
  void set_normalisation_probe(bool use) { use_norm_probe = use; }
  void set_per_sample_cb(std::function<void(Codec&)> f) { per_sample_cb = f; }
  void set_knob_changes(KnobChanges* k) { knob_changes = k; }
  void select_knob_changes(bool on) {track_knob_changes = on; }
  void set_adc_correction(std::function<uint16_t(uint16_t)> f) {adc_correction = f; adc_scale = calc_adc_scale(); };
  void select_adc_correction(uint bits) {adc_correct_mask = bits; };
  void select_adc_correction(ADCBitFlag bits) {select_adc_correction(static_cast<uint>(bits)); };
  void set_adc_scale(bool scale) {scale_adc = scale; };

  [[nodiscard]] uint16_t __not_in_flash_func(read_knob)(Knob k) { return knobs[Now][k]; }
  [[nodiscard]] uint16_t __not_in_flash_func(read_knob)(uint k) { return read_knob(static_cast<Knob>(k)); }
  [[nodiscard]] SwitchPosition __not_in_flash_func(read_switch)() { return static_cast<SwitchPosition>(knobs[Now][Switch]); }
  [[nodiscard]] bool __not_in_flash_func(knob_changed)(Knob k) { return knobs[Prev][k] != knobs[Now][k]; }
  [[nodiscard]] bool __not_in_flash_func(knob_changed)(uint k) { return knob_changed(static_cast<Knob>(k)); }

  [[nodiscard]] int16_t __not_in_flash_func(read_audio)(Channel lr) { return audio[1 - lr]; } // ports swapped
  [[nodiscard]] int16_t __not_in_flash_func(read_audio)(uint lr) { return read_audio(static_cast<Channel>(lr)); }
  void __not_in_flash_func(write_audio)(Channel lr, int16_t v) { cv_out[lr] = v; }
  void __not_in_flash_func(write_audio)(uint lr, int16_t v) { write_audio(static_cast<Channel>(lr), v); }

  [[nodiscard]] int16_t __not_in_flash_func(read_cv)(Channel lr) { return cv[lr]; }
  [[nodiscard]] int16_t __not_in_flash_func(read_cv)(uint lr) { return read_cv(static_cast<Channel>(lr)); }
  // cv pins in reverse order
  void __not_in_flash_func(write_cv)(Channel lr, int16_t v) {
    pwm_set_gpio_level(CV_OUT_1 - lr, scale_cv_out(static_cast<uint16_t>(0x800 - v)));
  }

  void __not_in_flash_func(write_cv)(uint lr, int16_t v) { write_cv(static_cast<Channel>(lr), v); }
  // these are used, for example, to write midi
  void __not_in_flash_func(write_cv)(Channel lr, uint16_t v) { pwm_set_gpio_level(CV_OUT_1 - lr, scale_cv_out(v)); }
  void __not_in_flash_func(write_cv)(uint lr, uint16_t v) { write_cv(static_cast<Channel>(lr), v); }

  [[nodiscard]] bool __not_in_flash_func(read_pulse)(Channel lr) { return pulse[lr]; }
  [[nodiscard]] bool __not_in_flash_func(read_pulse)(uint lr) { return read_pulse(static_cast<Channel>(lr)); }
  void __not_in_flash_func(write_pulse)(Channel lr, bool v) { gpio_put(PULSE_1_RAW_OUT + lr, !v); }
  void __not_in_flash_func(write_pulse)(uint lr, bool v) { write_pulse(static_cast<Channel>(lr), v); }
  // TODO - merge separate arrays
  [[nodiscard]] bool __not_in_flash_func(pulse_rose)(Channel lr) { return pulse[lr] && !last_pulse[lr]; }
  [[nodiscard]] bool __not_in_flash_func(pulse_rose)(uint lr) { return pulse_rose(static_cast<Channel>(lr)); }
  [[nodiscard]] bool __not_in_flash_func(pulse_fell)(Channel lr) { return !pulse[lr] && last_pulse[lr]; }
  [[nodiscard]] bool __not_in_flash_func(pulse_fell)(uint lr) { return pulse_fell(static_cast<Channel>(lr)); }

  [[nodiscard]] bool __not_in_flash_func(is_connected)(SocketIn i) { return connected[i]; }

private:

  static constexpr uint PULSE_1_RAW_OUT = 8;
  static constexpr uint PULSE_2_RAW_OUT = 9;
  static constexpr uint CV_OUT_1 = 23;
  static constexpr uint CV_OUT_2 = 22;
  static constexpr uint NORMALISATION_PROBE = 4;
  static constexpr uint MUX_LOGIC = 24;  // and 25
  static constexpr uint AUDIO_L_IN_1 = 27;
  static constexpr uint AUDIO_R_IN_1 = 26;
  static constexpr uint MUX_IN = 28;  // and 29
  static constexpr uint DAC_CHANNEL_A = 0x0000;
  static constexpr uint DAC_CHANNEL_B = 0x8000;
  static constexpr uint DAC_CS = 21;
  static constexpr uint DAC_SCK = 18;
  static constexpr uint DAC_TX = 19;
  static constexpr uint EEPROM_SDA = 16;
  static constexpr uint EEPROM_SCL = 17;
  static constexpr uint PULSE_1_INPUT = 2;
  static constexpr uint PULSE_2_INPUT = 3;
  static constexpr uint DEBUG_1 = 0;
  static constexpr uint DEBUG_2 = 1;

  static constexpr uint SPI_DREQ = DREQ_SPI0_TX;

  enum ADCRunMode { Running, ReqStop, Stopped, ReqStart };
  static constexpr uint N_PHASES = 2; // adc and cpu
  static constexpr uint N_MUX = 2;

  Codec();

  void buffer_full();
  std::function<void(Codec&)> per_sample_cb = [](Codec&) {};
  KnobChanges* knob_changes = nullptr;
  bool track_knob_changes = false;

  uint adc_correct_mask = 0;
  std::function<int16_t(uint16_t)> adc_correction = [](uint16_t adc) {return adc;};
  uint32_t adc_scale = calc_adc_scale();
  bool scale_adc = false;

  uint32_t count = 0;
  bool starting = false;
  volatile ADCRunMode run_mode;

  int16_t cv_out[N_CHANNELS] = {};
  volatile int32_t knobs[N_WHEN][N_KNOBS] = {};
  volatile bool pulse[N_CHANNELS] = {};
  volatile bool last_pulse[N_CHANNELS] = {};
  volatile int32_t cv[N_CHANNELS] = {};
  volatile int16_t audio[N_CHANNELS] = {0x800, 0x800};
  uint16_t adc_buffer[N_PHASES][4 * OVERSAMPLES] = {};
  uint16_t spi_buffer[N_PHASES][N_CHANNELS] = {};
  uint8_t adc_dma = 0, spi_dma = 0;

  bool use_norm_probe = false;
  volatile int32_t probe_in[N_SOCKET_IN] = {};
  volatile bool connected[N_SOCKET_IN] = {};
  static uint32_t next_norm_probe();
  static uint16_t dac_value(int16_t value, uint16_t dacChannel);
  static uint16_t scale_cv_out(uint16_t value);
  [[nodiscard]] uint32_t calc_adc_scale() const;
  [[nodiscard]] uint16_t apply_adc_scale(uint16_t v) const;

  static void audio_callback() {
    Codec& cc = Codec::get();
    cc.buffer_full();
  }
};


template <uint O, uint F> uint32_t Codec<O, F>::calc_adc_scale() const {
  const uint16_t adc_max = adc_correction(0xfff);
  return static_cast<uint32_t>((0xfff << 19) / adc_max);
}

template <uint O, uint F> uint16_t Codec<O, F>::apply_adc_scale(uint16_t v) const {
  return (v * adc_scale) >> 19;
}

// pseudo-random bit for normalisation probe
template <uint O, uint F>
uint32_t __attribute__((section(".time_critical." "cc-next-norm-probe")))
Codec<O, F>::next_norm_probe() {
  static uint32_t lcg_state = 1;
  lcg_state = 1664525 * lcg_state + 1013904223;
  return lcg_state >> 31;
}

template <uint O, uint F> uint16_t __attribute__((section(".time_critical." "dac-value")))
Codec<O, F>::dac_value(int16_t value, uint16_t dacChannel) {
  // cc had more complex logic here so i may be missing something
  return (dacChannel | 0x3000) | (0xfff & static_cast<uint16_t>(value + 0x800));
}

template <uint O, uint F> uint16_t __attribute__((section(".time_critical." "cv-out")))
Codec<O, F>::scale_cv_out(uint16_t v) {
  return v >> 1; // pwm is 11 bits to reduce ripple
}

template <uint O, uint SAMPLE_FREQ> void __attribute__((section(".time_critical." "cc-audio-worker")))
Codec<O, SAMPLE_FREQ>::start() {
  count = 0;
  starting = true;

  const uint dma_phase = count & 0x1;
  adc_select_input(0);
  adc_set_round_robin(0b0001111U);
  adc_fifo_setup(true, true, 1, false, false);
  adc_set_clkdiv(48000000 / (SAMPLE_FREQ * 4.0 * OVERSAMPLES) - 1);
  adc_dma = dma_claim_unused_channel(true);
  spi_dma = dma_claim_unused_channel(true);
  dma_channel_config adc_dmacfg = dma_channel_get_default_config(adc_dma);
  dma_channel_config spi_dmacfg = dma_channel_get_default_config(spi_dma);
  channel_config_set_transfer_data_size(&adc_dmacfg, DMA_SIZE_16);
  channel_config_set_read_increment(&adc_dmacfg, false);
  channel_config_set_write_increment(&adc_dmacfg, true);
  channel_config_set_dreq(&adc_dmacfg, DREQ_ADC);
  dma_channel_configure(adc_dma, &adc_dmacfg, adc_buffer[dma_phase], &adc_hw->fifo, 4 * OVERSAMPLES, true);
  dma_channel_set_irq0_enabled(adc_dma, true);

  irq_set_enabled(DMA_IRQ_0, true);
  irq_set_exclusive_handler(DMA_IRQ_0, audio_callback);

  spi_dmacfg = dma_channel_get_default_config(spi_dma);
  channel_config_set_transfer_data_size(&spi_dmacfg, DMA_SIZE_16);
  channel_config_set_dreq(&spi_dmacfg, SPI_DREQ);
  dma_channel_configure(spi_dma, &spi_dmacfg, &spi_get_hw(spi0)->dr, nullptr, 2, false);

  adc_run(true);

  while (true) {
    if (run_mode == ReqStart) {
      run_mode = Running;

      dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
      dma_channel_set_write_addr(adc_dma, adc_buffer[dma_phase], true); // start writing into new buffer
      dma_channel_set_read_addr(spi_dma, spi_buffer[dma_phase], true); // start reading from new buffer

      adc_set_round_robin(0);
      adc_select_input(0);
      adc_set_round_robin(0b0001111U);
      adc_run(true);
    } else if (run_mode == Stopped) {
      break;
    }
  }
}

template <uint O, uint F> void Codec<O, F>::stop() {
  run_mode = ReqStop;
}

template <uint OVERSAMPLE_BITS, uint F> __attribute__((section(".time_critical." "cc-buffer-full")))
void Codec<OVERSAMPLE_BITS, F>::buffer_full() {
  uint mux_state = count & 0x3;
  uint norm_probe_count = count & 0xf;
  uint cpu_phase = count & 0x1;
  uint dma_phase = 1 - cpu_phase;
  static int probe_out = 0;

  static volatile int32_t smooth_knobs[N_KNOBS] = {};
  static volatile int32_t smooth_cv[N_CHANNELS] = {};

  adc_select_input(0); // TODO - why is this here?

  const uint next_mux_state = (mux_state + 1) & 0x3;
  for (uint mux = 0; mux < N_MUX; mux++) gpio_put(MUX_LOGIC + mux, next_mux_state & (1 << mux));

  dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
  dma_channel_set_write_addr(adc_dma, adc_buffer[dma_phase], true); // start writing into new buffer
  dma_channel_set_read_addr(spi_dma, spi_buffer[dma_phase], true); // start reading from new buffer

  const uint cv_lr = mux_state & 1;
  smooth_cv[cv_lr] = (15 * (smooth_cv[cv_lr]) + 16 * adc_buffer[cpu_phase][3]) >> 4; // 240hz lpf
  uint16_t cv_tmp = smooth_cv[cv_lr] >> 4;
  if (adc_correct_mask & (C1 << cv_lr)) {
    cv_tmp = adc_correction(cv_tmp);
    if (scale_adc) cv_tmp = apply_adc_scale(cv_tmp);
  }
  cv[cv_lr] = 0x800 - cv_tmp;

  // TODO - this puts an upper limit on OVERSAMPLE_BITS (could use int32_t temp)
  for (uint audio_lr = 0; audio_lr < N_CHANNELS; audio_lr++) {
    uint16_t audio_tmp = 0;
    for (uint i = 0; i < OVERSAMPLES; ++i) audio_tmp += adc_buffer[cpu_phase][audio_lr + 4 * i];
    audio_tmp >>= OVERSAMPLE_BITS;
    if (adc_correct_mask & (A1 << audio_lr)) {
      audio_tmp = adc_correction(audio_tmp);
      if (scale_adc) audio_tmp = apply_adc_scale(audio_tmp);
    }
    audio[audio_lr] = static_cast<int16_t>(0x800 - audio_tmp);
  }

  for (uint pulse_lr = 0; pulse_lr < N_CHANNELS; pulse_lr++) {
    last_pulse[pulse_lr] = pulse[pulse_lr];
    pulse[pulse_lr] = !gpio_get(PULSE_1_INPUT + pulse_lr); // TODO - assumes sequential port, should we flag somehow?
  }

  const uint knob = mux_state;
  smooth_knobs[knob] = (127 * smooth_knobs[knob] + 16 * (adc_buffer[cpu_phase][2] >> 4)) >> 7; // 60hz lpf
  knobs[Prev][knob] = knobs[Now][knob];
  if (knob == Switch) {
    knobs[Now][Switch] = (smooth_knobs[Switch] > 1000) + (smooth_knobs[Switch] > 3000);
  } else {
    knobs[Now][knob] = smooth_knobs[knob];
  }

  if (starting) {  // avoid startup noise
    knobs[Prev][knob] = knobs[Now][knob];
    // TODO - Should initialise knob and CV smoothing filters here too
    starting = count < 8;
  }

  if (use_norm_probe) {
    // this seems to send a random signal to all inputs, with a new bit sent every 16 cycles.
    // if we read in the same random sequence then we know that the socket is not connected
    // (presumably a connected socket reads the connect signal which will not match)
    if (norm_probe_count == 0) {
      const int32_t next_probe_out_bit = next_norm_probe();
      gpio_put(NORMALISATION_PROBE, next_probe_out_bit);
      probe_out = (probe_out << 1) + next_probe_out_bit;
    }
    if (norm_probe_count == 14 || norm_probe_count == 15) {
      probe_in[2 + cv_lr] = (probe_in[2 + cv_lr] << 1) + (adc_buffer[cpu_phase][3] < 1800);
    }
    if (norm_probe_count == 15) {
      probe_in[SocketIn::Audio1] = (probe_in[SocketIn::Audio1] << 1) + (adc_buffer[cpu_phase][1] < 1800);
      probe_in[SocketIn::Audio2] = (probe_in[SocketIn::Audio2] << 1) + (adc_buffer[cpu_phase][0] < 1800);
      probe_in[SocketIn::Pulse1] = (probe_in[SocketIn::Pulse1] << 1) + (pulse[0]);
      probe_in[SocketIn::Pulse2] = (probe_in[SocketIn::Pulse2] << 1) + (pulse[1]);
      for (uint i = 0; i < N_SOCKET_IN; i++) connected[i] = (probe_out != probe_in[i]);
    }

    // Force disconnected values to zero, rather than the normalisation probe garbage
    if (!is_connected(SocketIn::Audio1)) audio[1] = 0;
    if (!is_connected(SocketIn::Audio2)) audio[0] = 0;
    if (!is_connected(SocketIn::CV1)) cv[0] = 0;
    if (!is_connected(SocketIn::CV2)) cv[1] = 0;
    if (!is_connected(SocketIn::Pulse1)) pulse[0] = false;
    if (!is_connected(SocketIn::Pulse2)) pulse[1] = false;
  }

  if (track_knob_changes && knob_changed(knob) && knob_changes) {
    knob_changes->handle_knob_change(knob, knobs[Now][knob], knobs[Prev][knob]);
  }
  per_sample_cb(*this); // user callback

  // invert to counteract inverting output configuration
  spi_buffer[cpu_phase][0] = dac_value(-cv_out[0], DAC_CHANNEL_A);
  spi_buffer[cpu_phase][1] = dac_value(-cv_out[1], DAC_CHANNEL_B);

  if (run_mode == ReqStop) {
    adc_run(false);
    adc_set_round_robin(0);
    adc_select_input(0);
    dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
    dma_channel_cleanup(adc_dma);
    dma_channel_cleanup(spi_dma);
    irq_remove_handler(DMA_IRQ_0, Codec::audio_callback);
    run_mode = Stopped;
  }

  count++;
}

template <uint O, uint F> Codec<O, F>::Codec() {
  run_mode = Running;
  adc_run(false);
  adc_select_input(0);

  use_norm_probe = false;
  for (uint i = 0; i < N_SOCKET_IN; i++) connected[i] = false;

  gpio_init(NORMALISATION_PROBE);
  gpio_set_dir(NORMALISATION_PROBE, GPIO_OUT);
  gpio_put(NORMALISATION_PROBE, false);

  adc_init();

  adc_gpio_init(AUDIO_L_IN_1);
  adc_gpio_init(AUDIO_R_IN_1);
  for (uint mux = 0; mux < N_MUX; mux++) adc_gpio_init(MUX_IN + mux);

  for (uint mux = 0; mux < 2; mux++) {
    gpio_init(MUX_LOGIC + mux);
    gpio_set_dir(MUX_LOGIC + mux, GPIO_OUT);
  }

  gpio_init(PULSE_1_RAW_OUT);
  gpio_set_dir(PULSE_1_RAW_OUT, GPIO_OUT);
  gpio_put(PULSE_1_RAW_OUT, true); // raw high (output low)
  gpio_init(PULSE_2_RAW_OUT);
  gpio_set_dir(PULSE_2_RAW_OUT, GPIO_OUT);
  gpio_put(PULSE_2_RAW_OUT, true);

  gpio_init(PULSE_1_INPUT);
  gpio_set_dir(PULSE_1_INPUT, GPIO_IN);
  gpio_pull_up(PULSE_1_INPUT); // NB Needs pullup to activate transistor on inputs
  gpio_init(PULSE_2_INPUT);
  gpio_set_dir(PULSE_2_INPUT, GPIO_IN);
  gpio_pull_up(PULSE_2_INPUT); // NB: Needs pullup to activate transistor on inputs

  spi_init(spi0, 15625000);
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(DAC_SCK, GPIO_FUNC_SPI);
  gpio_set_function(DAC_TX, GPIO_FUNC_SPI);
  gpio_set_function(DAC_CS, GPIO_FUNC_SPI);

  i2c_init(i2c0, 100 * 1000);
  gpio_set_function(EEPROM_SDA, GPIO_FUNC_I2C);
  gpio_set_function(EEPROM_SCL, GPIO_FUNC_I2C);

  gpio_set_function(CV_OUT_1, GPIO_FUNC_PWM);
  gpio_set_function(CV_OUT_2, GPIO_FUNC_PWM);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_wrap(&config, 0x7ff); // 11-bit PWM
  // CV_A and CV_B share the same PWM slice, which means that they share a PWM config
  // they have separate 'gpio_level's (output compare unit) though, so they can have different PWM on-times
  pwm_init(pwm_gpio_to_slice_num(CV_OUT_1), &config, true); // slice 1, channel A
  pwm_init(pwm_gpio_to_slice_num(CV_OUT_2), &config, true); // slice 1, channel B (redundant to set up again)
  pwm_set_gpio_level(CV_OUT_1, scale_cv_out(0x800));
  pwm_set_gpio_level(CV_OUT_2, scale_cv_out(0x800));
}

#endif
