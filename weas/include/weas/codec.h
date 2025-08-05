
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


// the interface to the DAC and ADC - hardware logic largely from ComputerCard.h
// main extensions are:
// * selectable oversampling and sample frequency
// * selectable ADC correction (DNL issues), scaling, and masks
// * modified filtering of CV (1/10 nyquist) and knobs (1/100 nyquist)
// * callback for knob events (see fifo.h)

// separated into two classes:
// * Codec is the interface used elsewhere, but cannot be instantiated directly
// * CodecFactory is a template class that creates instances of Codec and talks to the hardware
// the separation is purely to avoid having templates all over the place.
// downcast the CodecFactory singleton to a Codec and pass that by reference
// (the CodeFactory instance adds no API interesting to the end user).


static constexpr uint CODEC_SAMPLE_44_1 = 44100;
static constexpr uint CODEC_SAMPLE_48 = 48000;
static std::function<int16_t(uint16_t)> CODEC_NULL_CORRECTION = [](uint16_t adc) {return adc;};


class Codec {

public:

  enum Knob { Main, X, Y, Switch };
  enum SwitchPosition { Up, Middle, Down };  // order matches leds_direct.h2
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
  enum ADCSource { Audios, CVs, Knobs };
  static constexpr uint N_ADC_SOURCES = Knobs + 1;

  Codec(const Codec&) = delete;
  Codec& operator=(const Codec&) = delete;
  virtual ~Codec() = default;

  virtual void start() = 0;
  void stop(); // TODO - example of use?

  [[nodiscard]] int32_t get_count() const { return count; }
  void set_normalisation_probe(bool use) { use_norm_probe = use; }
  void set_per_sample_cb(std::function<void(Codec&)> f) { per_sample_cb = f; }
  void set_knob_changes(KnobChanges* k) { knob_changes = k; }
  void select_knob_changes(bool on) {track_knob_changes = on; }
  void set_adc_correction_and_scale(std::function<uint16_t(uint16_t)> f) {adc_correction = f; adc_scale = calc_adc_scale(); };
  void select_adc_correction(uint bits) {adc_correct_mask = bits; };
  void select_adc_correction(ADCBitFlag bits) {select_adc_correction(static_cast<uint>(bits)); };
  void select_adc_scale(bool scale) {scale_adc = scale; };
  void set_adc_mask(ADCSource s, uint16_t mask) {adc_mask[s] = mask; };
  void set_adc_mask(uint s, uint16_t mask) {set_adc_mask(static_cast<ADCSource>(s), mask); };
  // use a bigger number if you lower SAMPLE_FREQ and knobs become sluggish
  void set_knob_alpha(uint a) {knob_alpha = std::min(6u, std::max(1u, a)); }

  [[nodiscard]] uint16_t __not_in_flash_func(read_knob)(Knob k) { return knobs[Now][k]; }
  [[nodiscard]] uint16_t __not_in_flash_func(read_knob)(uint k) { return read_knob(static_cast<Knob>(k)); }
  [[nodiscard]] SwitchPosition __not_in_flash_func(read_switch)() { return static_cast<SwitchPosition>(knobs[Now][Switch]); }
  [[nodiscard]] bool __not_in_flash_func(knob_changed)(Knob k) { return knobs[Prev][k] != knobs[Now][k]; }
  [[nodiscard]] bool __not_in_flash_func(knob_changed)(uint k) { return knob_changed(static_cast<Knob>(k)); }

  void set_connected_changes(KnobChanges* k) { knob_changes = k; }
  void select_connected_changes(bool on) {track_connected_changes = on; }
  [[nodiscard]] int16_t __not_in_flash_func(read_audio)(Channel lr) { return audio[1 - lr]; } // ports swapped
  [[nodiscard]] int16_t __not_in_flash_func(read_audio)(uint lr) { return read_audio(static_cast<Channel>(lr)); }
  void __not_in_flash_func(write_audio)(Channel lr, int16_t v) { audio_out[lr] = v; }
  void __not_in_flash_func(write_audio)(uint lr, int16_t v) { write_audio(static_cast<Channel>(lr), v); }

  [[nodiscard]] int16_t __not_in_flash_func(read_cv)(Channel lr) { return cv[lr]; }
  [[nodiscard]] int16_t __not_in_flash_func(read_cv)(uint lr) { return read_cv(static_cast<Channel>(lr)); }

  // cv pins in reverse order
  // 12 bit signed
  void __not_in_flash_func(write_cv)(Channel lr, int16_t v) {
    cv_out[lr] = (2047 - std::min(2047, std::max(-2048, static_cast<int>(v)))) << 7;
  }
  void __not_in_flash_func(write_cv)(uint lr, int16_t v) { write_cv(static_cast<Channel>(lr), v); }
  // 19 bit signed
  void __not_in_flash_func(write_cv)(Channel lr, int32_t v) {
    cv_out[lr] = 262143 - std::min(262143, std::max(-262144, static_cast<int>(v)));
  }
  void __not_in_flash_func(write_cv)(uint lr, int32_t v) { write_cv(static_cast<Channel>(lr), v); }
  // these are used, for example, to write midi
  void __not_in_flash_func(write_cv)(Channel lr, uint32_t v) { cv_out[lr] = v; }
  void __not_in_flash_func(write_cv)(uint lr, uint32_t v) { write_cv(static_cast<Channel>(lr), v); }

  [[nodiscard]] bool __not_in_flash_func(read_pulse)(Channel lr) { return pulse[lr]; }
  [[nodiscard]] bool __not_in_flash_func(read_pulse)(uint lr) { return read_pulse(static_cast<Channel>(lr)); }
  void __not_in_flash_func(write_pulse)(Channel lr, bool v) { gpio_put(PULSE_RAW_OUT + lr, !v); }
  void __not_in_flash_func(write_pulse)(uint lr, bool v) { write_pulse(static_cast<Channel>(lr), v); }
  // TODO - merge separate arrays
  [[nodiscard]] bool __not_in_flash_func(pulse_rose)(Channel lr) { return pulse[lr] && !last_pulse[lr]; }
  [[nodiscard]] bool __not_in_flash_func(pulse_rose)(uint lr) { return pulse_rose(static_cast<Channel>(lr)); }
  [[nodiscard]] bool __not_in_flash_func(pulse_fell)(Channel lr) { return !pulse[lr] && last_pulse[lr]; }
  [[nodiscard]] bool __not_in_flash_func(pulse_fell)(uint lr) { return pulse_fell(static_cast<Channel>(lr)); }

  [[nodiscard]] bool __not_in_flash_func(is_connected)(SocketIn i) { return connected[Now][i]; }
  [[nodiscard]] bool __not_in_flash_func(is_connected)(uint i) { return is_connected(static_cast<SocketIn>(i)); }
  [[nodiscard]] bool __not_in_flash_func(connected_changed)(SocketIn i) { return connected[Now][i] != connected[Prev][i]; }
  [[nodiscard]] bool __not_in_flash_func(connected_changed)(uint i) { return connected_changed(static_cast<SocketIn>(i)); }

protected:

  static constexpr uint PULSE_RAW_OUT = 8;  // and 9
  static constexpr uint CV_OUT = 22;  // and 23 lr swapped
  static constexpr uint NORMALISATION_PROBE = 4;
  static constexpr uint MUX_LOGIC = 24;  // and 25
  static constexpr uint AUDIO_IN = 26;  // and 27 lr swapped
  static constexpr uint MUX_IN = 28;  // and 29
  static constexpr uint DAC_CHANNEL_A = 0x0000;
  static constexpr uint DAC_CHANNEL_B = 0x8000;
  static constexpr uint DAC_CS = 21;
  static constexpr uint DAC_SCK = 18;
  static constexpr uint DAC_TX = 19;
  static constexpr uint EEPROM_SDA = 16;
  static constexpr uint EEPROM_SCL = 17;
  static constexpr uint PULSE_IN = 2;  // and 3
  static constexpr uint DEBUG_1 = 0;
  static constexpr uint DEBUG_2 = 1;

  static constexpr uint SPI_DREQ = DREQ_SPI0_TX;

  enum ADCRunMode { Running, ReqStop, Stopped, ReqStart };
  static constexpr uint N_PHASES = 2; // adc and cpu
  static constexpr uint N_MUX = 2;

  Codec() = default;

  std::function<void(Codec&)> per_sample_cb = [](Codec&) {};

  KnobChanges* knob_changes = nullptr;
  bool track_knob_changes = false;
  uint knob_alpha = 1;
  static constexpr uint NOISE_REDN = 4;

  uint adc_correct_mask = 0;
  std::function<int16_t(uint16_t)> adc_correction = CODEC_NULL_CORRECTION;
  uint32_t adc_scale = calc_adc_scale();
  bool scale_adc = false;
  uint16_t adc_mask[N_ADC_SOURCES] = { 0xffffu, 0xffffu, 0xffffu };  // TODO - hardcodes N_AUDIO_SOURCES

  uint32_t count = 0;
  bool starting = false;
  volatile ADCRunMode run_mode;

  uint32_t cv_out[N_CHANNELS] = {262144u, 262144u};  // TODO - again, hardcoding length
  uint16_t audio_out[N_CHANNELS] = {};
  volatile int16_t knobs[N_WHEN][N_KNOBS] = {};
  volatile bool pulse[N_CHANNELS] = {};
  volatile bool last_pulse[N_CHANNELS] = {};
  volatile int16_t cv[N_CHANNELS] = {};
  volatile int16_t audio[N_CHANNELS] = {0x800, 0x800};
  uint16_t spi_buffer[N_PHASES][N_CHANNELS] = {};
  uint8_t adc_dma = 0, spi_dma = 0;

  ConnectedChanges* connected_changes = nullptr;
  bool track_connected_changes = false;
  bool use_norm_probe = false;
  volatile uint32_t probe_in[N_SOCKET_IN] = {};
  volatile bool connected[N_WHEN][N_SOCKET_IN] = {};
  static uint32_t next_norm_probe();
  static uint16_t dac_value(int16_t value, uint16_t dacChannel);
  static uint16_t scale_cv_out(uint16_t value);
  [[nodiscard]] uint32_t calc_adc_scale();
  [[nodiscard]] uint16_t apply_adc_scale(uint16_t v);

  // longer method definitions in codec.cpp

};


// this is a templated singleton that subclasses Codec.  usage:
//   Codec& codec = CodecFactory<OBITS, FREQ>::get();
// then use codec as required (typically via set_per_sample_cb()).

template <uint OVERSAMPLE_BITS, uint SAMPLE_FREQ>
class CodecFactory : public Codec {

public:

  static constexpr uint OVERSAMPLES = 1 << OVERSAMPLE_BITS;

  CodecFactory(const CodecFactory&) = delete;
  CodecFactory& operator=(const CodecFactory&) = delete;

  void start() override;

  static CodecFactory& get() {
    static CodecFactory cf;
    return cf;
  }

private:

  CodecFactory();

  void handle_adc();
  void handle_cv();
  uint16_t adc_buffer[N_PHASES][4 * OVERSAMPLES] = {};

  static void adc_callback() {
    CodecFactory& cf = CodecFactory::get();
    cf.handle_adc();
  }

  static void cv_callback() {
    CodecFactory& cf = CodecFactory::get();
    cf.handle_cv();
  }
};

template <uint O, uint S> CodecFactory<O, S>::CodecFactory() {
  run_mode = Running;
  adc_run(false);
  adc_select_input(0);

  use_norm_probe = false;
  for (uint i = 0; i < N_SOCKET_IN; i++) {
    connected[Prev][i] = connected[Now][i];
    connected[Now][i] = false;
  }

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
    gpio_init(PULSE_IN + lr);
    gpio_set_dir(PULSE_IN + lr, GPIO_IN);
    gpio_pull_up(PULSE_IN + lr); // needs pullup to activate transistor on inputs
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

template <uint O, uint SAMPLE_FREQ> void __attribute__((section(".time_critical." "cc-start")))
CodecFactory<O, SAMPLE_FREQ>::start() {

  count = 0;
  starting = true;

  const uint dma_phase = count & 0x1;
  adc_select_input(0);
  adc_set_round_robin(0b0001111U);
  adc_fifo_setup(true, true, 1, false, false);
  // this is clamped at 96 clock ticks.
  // that implies the maximum value of f x n is 125,000
  // so a sample freq (f) of 48khz implies n of 2
  // for oversample (n) of 4, f must be 31.25khz (nyquist 16khz)
  // in practice, if you ask for too much you get the oversampling you asked
  // for at the highest rate possible.
  adc_set_clkdiv(48000000 / (SAMPLE_FREQ * 4.0f * OVERSAMPLES) - 1);
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
  irq_set_exclusive_handler(DMA_IRQ_0, adc_callback);

  uint slice_num = pwm_gpio_to_slice_num(CV_OUT);
  pwm_clear_irq(slice_num);
  pwm_set_irq_enabled(slice_num, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, cv_callback);
  irq_set_priority(PWM_IRQ_WRAP, 255);
  irq_set_enabled(PWM_IRQ_WRAP, true);

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
      irq_set_enabled(PWM_IRQ_WRAP, false);
      pwm_clear_irq(pwm_gpio_to_slice_num(CV_OUT));
      irq_remove_handler(PWM_IRQ_WRAP, cv_callback);
      break;
    }
  }
}

template <uint OVERSAMPLE_BITS, uint F> __attribute__((section(".time_critical." "cc-handle-adc")))
void CodecFactory<OVERSAMPLE_BITS, F>::handle_adc() {

  uint mux_state = count & 0x3;
  uint norm_probe_count = count & 0xf;
  uint cpu_phase = count & 0x1;
  uint dma_phase = 1 - cpu_phase;
  static uint32_t probe_out = 0;

  static volatile uint32_t smooth_knobs[N_KNOBS] = {};  // 32 bits to provide room for NOISE_REDN
  static volatile uint32_t smooth_cv[N_CHANNELS] = {};

  adc_select_input(0); // TODO - why is this here?

  const uint next_mux_state = (mux_state + 1) & 0x3;
  for (uint mux = 0; mux < N_MUX; mux++) gpio_put(MUX_LOGIC + mux, next_mux_state & (1 << mux));

  dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
  dma_channel_set_write_addr(adc_dma, adc_buffer[dma_phase], true); // start writing into new buffer
  dma_channel_set_read_addr(spi_dma, spi_buffer[dma_phase], true); // start reading from new buffer

  const uint cv_lr = mux_state & 1;

  // context: from google or https://dsp.stackexchange.com/questions/40462/exponential-moving-average-cut-off-frequency
  // (but note that the arccos(x) is replaced by x (series expansion for cos)) for a EWMA with coeffs alpha and
  // 1-alpha (ie F(t) = alpha x + (1 - alpha) F(t - 1)) the cutoff freq f = sample_freq x alpha / (2 pi) for small alpha
  // or sample_freq * alpha / ((1 - alpha) (2 pi)) for larger alpha.

  // ComputerCard code:
  //   smooth_cv[cv_lr] = (15 * (smooth_cv[cv_lr]) + 16 * adc_buffer[cpu_phase][3]) >> 4;  // 240hz lpf
  //   cv[cv_lr] = smooth_cv[cv_lr] >> 4
  // this has  the input shifted up by 4 bits (to reduce noise) which is dropped later.
  // so alpha here is 1/16 and f = 24khz / (16 x 6) = 250hz (remember cv is sampled every other cycle).

  // here, the sample freq varies.  why not aim for, say, 1/10 audio nyquist?  then we don't need to adjust
  // for sample freq (since the two scale together).  cv is already 1/2 cycles (ie audio nyquist),
  // so alpha/2pi = 1/10, but then alpha is no longer small, and using the other formula we get alpha = 1/3
  // and keep an extra 4 bits for noise:

  smooth_cv[cv_lr] = (21 * smooth_cv[cv_lr] + 11 * (adc_buffer[cpu_phase][3] << NOISE_REDN)) >> 5;
  uint16_t cv_tmp = smooth_cv[cv_lr] >> NOISE_REDN;
  if (adc_correct_mask & (C1 << cv_lr)) {
    cv_tmp = adc_correction(cv_tmp);
    if (scale_adc) cv_tmp = apply_adc_scale(cv_tmp);
  }
  cv[cv_lr] = static_cast<int16_t>(0x800 - (cv_tmp & adc_mask[CVs]));

  for (uint audio_lr = 0; audio_lr < N_CHANNELS; audio_lr++) {
    uint32_t audio_tmp_wide = 0;
    for (uint i = 0; i < OVERSAMPLES; ++i) audio_tmp_wide += adc_buffer[cpu_phase][audio_lr + 4 * i];
    auto audio_tmp = static_cast<uint16_t>(audio_tmp_wide >> OVERSAMPLE_BITS);
    if (adc_correct_mask & (A1 << audio_lr)) {
      audio_tmp = adc_correction(audio_tmp);
      if (scale_adc) audio_tmp = apply_adc_scale(audio_tmp);
    }
    audio[audio_lr] = static_cast<int16_t>(0x800 - (audio_tmp & adc_mask[Audios]));
  }

  for (uint pulse_lr = 0; pulse_lr < N_CHANNELS; pulse_lr++) {
    last_pulse[pulse_lr] = pulse[pulse_lr];
    pulse[pulse_lr] = !gpio_get(PULSE_IN + pulse_lr);
  }

  const uint knob = mux_state;
  // see discussion above.  if we aim for 1/100 nyquist (240hz), but raw data already 1/4 cycles,
  // so alpha/(2pi(1-alpha)) = 1/25, alpha = 6/31 but that's a bit noisy at 48khs.  how low can we go?
  // an alpha of 1/32 would be 1/200 x 12khz = 60hz - that's a decent range, so let's make it configurable
  smooth_knobs[knob] = ((32 - knob_alpha) * smooth_knobs[knob] + knob_alpha * (adc_buffer[cpu_phase][2] << NOISE_REDN)) >> 5;
  knobs[Prev][knob] = knobs[Now][knob];
  if (knob == Switch) {
    knobs[Now][Switch] = 2 - (smooth_knobs[Switch] > (1000 << NOISE_REDN)) - (smooth_knobs[Switch] > (3000 << NOISE_REDN));
  } else {
    knobs[Now][knob] = (smooth_knobs[knob] >> NOISE_REDN) & adc_mask[Knobs];
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
      const uint32_t next_probe_out_bit = next_norm_probe();
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
      for (uint i = 0; i < N_SOCKET_IN; i++) connected[Now][i] = (probe_out != probe_in[i]);
    }

    // Force disconnected values to zero, rather than the normalisation probe garbage
    if (!is_connected(SocketIn::Audio1)) audio[1] = 0;
    if (!is_connected(SocketIn::Audio2)) audio[0] = 0;
    if (!is_connected(SocketIn::CV1)) cv[0] = 0;
    if (!is_connected(SocketIn::CV2)) cv[1] = 0;
    if (!is_connected(SocketIn::Pulse1)) pulse[0] = false;
    if (!is_connected(SocketIn::Pulse2)) pulse[1] = false;
  }

  if (!starting) {
    if (track_knob_changes && knob_changed(knob) && knob_changes) {
      knob_changes->handle_knob_change(knob, knobs[Now][knob], knobs[Prev][knob]);
    }
    if (track_connected_changes && connected_changes) {
      for (uint skt = 0; skt < N_SOCKET_IN; skt++) {
        if (connected_changed(skt)) connected_changes->handle_connected_change(skt, connected[Now][skt]);
      }
    }
    per_sample_cb(*this); // user callback
  }

  // invert to counteract inverting output configuration
  spi_buffer[cpu_phase][0] = dac_value(-audio_out[0], DAC_CHANNEL_A);  // TODO
  spi_buffer[cpu_phase][1] = dac_value(-audio_out[1], DAC_CHANNEL_B);

  if (run_mode == ReqStop) {
    adc_run(false);
    adc_set_round_robin(0);
    adc_select_input(0);
    dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
    dma_channel_cleanup(adc_dma);
    dma_channel_cleanup(spi_dma);
    irq_set_enabled(DMA_IRQ_0, false);
    irq_remove_handler(DMA_IRQ_0, adc_callback);
    run_mode = Stopped;
  }

  count++;
}

template <uint OVERSAMPLE_BITS, uint F> __attribute__((section(".time_critical." "cc-handle-cv")))
void CodecFactory<OVERSAMPLE_BITS, F>::handle_cv() {

  // TODO - understand this
  // note channels swapped because CV pins swapped
  pwm_clear_irq(pwm_gpio_to_slice_num(CV_OUT)); // clear the interrupt flag

  static int32_t error_0 = 0, error_1 = 0;
  uint32_t truncated_cv1_val = (cv_out[1] - error_1) & 0xFFFFFF00;
  error_1 += truncated_cv1_val - cv_out[1];
  pwm_set_gpio_level(CV_OUT, truncated_cv1_val >> 8);

  uint32_t truncated_cv0_val = (cv_out[0] - error_0) & 0xFFFFFF00;
  error_0 += truncated_cv0_val - cv_out[0];
  pwm_set_gpio_level(CV_OUT + 1, truncated_cv0_val >> 8);
}


#endif
