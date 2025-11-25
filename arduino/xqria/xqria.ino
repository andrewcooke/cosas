
#include <Arduino.h>
#include <atomic>
#include <array>
#include <numeric>
#include <mutex>
#include <thread>
#include "esp_timer.h"
#include "driver/dac_continuous.h"
#include "math.h"
#include <Preferences.h>

const uint DMA_BUFFER_SIZE = 4092;  // 32 to 4092, multiple of 4; padded 16 bit; audible artefacts at 256 (even 1024) and below and i don't understand why
const uint MAX_LOCAL_BUFFER_SIZE = DMA_BUFFER_SIZE / 2;  // 8 bit
const uint MIN_LOCAL_BUFFER_SIZE = 10;  // anything lower grinds
const uint SAMPLE_RATE_HZ = 40000;
const uint LOWEST_F_HZ = 20;
const uint N_SAMPLES = SAMPLE_RATE_HZ / LOWEST_F_HZ;
const uint PHASE_EXTN = 2;  // extra bits for phase to allow better resolution at low f  TODO - what limits this?
const uint N_SAMPLES_EXTN = N_SAMPLES << PHASE_EXTN;
const uint BEAT_SCALE = SAMPLE_RATE_HZ * 60;  // ticks for 1 bpm
const std::array<uint, 16> SUBDIVS = {5, 10, 12, 15, 20, 24, 25, 30, 35, 36, 40, 45, 48, 50, 55, 60};  // by luck length is power of 2

// used in crash sound
const uint NOISE_BITS = 6;
const uint N_NOISE = 1 << NOISE_BITS;
const uint NOISE_MASK = N_NOISE - 1;

const uint LED_FREQ = 1000;
const uint LED_BITS = 8;

// global parameters that can be changed during use
volatile static uint BPM = 90;
volatile static uint SUBDIV_IDX = 15;
volatile static uint COMP_BITS = 0;
volatile static uint ENABLED = 10;  // all enabled (gray(10) = 9xf)
volatile static uint LOCAL_BUFFER_SIZE = 1024;  // has to be even
volatile static uint REFRESH_US = (1000000 * LOCAL_BUFFER_SIZE) / SAMPLE_RATE_HZ;

const uint N6 = 1 << 6;
const uint MAX6 = N6 - 1;
const uint N7 = 1 << 7;
const uint MAX7 = N7 - 1;
const uint N8 = 1 << 8;
const uint MAX8 = N8 - 1;
const uint N9 = 1 << 9;
const uint MAX9 = N9 - 1;
const uint N10 = 1 << 10;
const uint MAX10 = N10 - 1;
const uint N11 = 1 << 11;
const uint MAX11 = N11 - 1;
const uint N12 = 1 << 12;
const uint MAX12 = N12 - 1;

const uint DBG_LOTTERY = 10000;
const bool DBG_VOICE = false;
const bool DBG_LFSR = false;
const bool DBG_VOLUME = false;
const bool DBG_COMP = false;
const bool DBG_TIMING = false;
const bool DBG_FM = false;
const bool DBG_BEEP = false;
const bool DBG_DRUM = false;
const bool DBG_CRASH = false;
const bool DBG_MINIFM = false;
const bool DBG_REVERB = false;
const bool DBG_TONE = false;
const bool DBG_EUCLIDEAN = true;
const bool DBG_JIGGLE = false;
const bool DBG_PATTERN = false;
const bool DBG_COPY = true;

struct VoiceData {
  uint amp;
  uint freq;
  uint durn;
  uint fm;
  int shift;
};

struct VaultData {
  uint n_places;
  float frac_beats;
  float frac_main;
  uint prob;
  uint voice;
};

struct AllData {
  VoiceData voices[4];
  VaultData vaults[2];
  uint reverb_size;
  uint reverb_head_num;
  uint bpm;
  uint subdiv_idx;
  uint comp_bits;
  uint local_buffer_size;
};

template<typename T> int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

template<typename T> T series_exp(T val, uint n) {
  T result = 1;
  T fact = 1;
  T nth = val;
  for (uint i = 0; i < n; i++) {
    result += nth / fact;
    nth *= val;
    fact *= (i + 2);
  }
  return result;
}

// efficient random bits from an LFSR (random quality is not important here!)
class LFSR16 {
private:
  uint16_t state;
public:
  LFSR16(uint16_t seed = 0xACE1)
    : state(seed) {}
  uint next() {
    uint16_t lsb = state & 0x1;
    state >>= 1;
    if (lsb) state ^= 0xB400;
    return lsb;
  }
  int n_bits(uint n) {
    int bits = un_bits(n);
    // remove most of the bias (shift to be around zero rather than all +ve)
    if (n > 0) bits -= 1 << (n - 1);
    return bits;
  }
  uint un_bits(uint n) {
    static uint line = 0;
    int bits = 0;
    while (n) {
      uint bit = next();
      if (DBG_LFSR) {
        Serial.print(bit ? "1" : "0");
        if (line++ == 80) {
          line = 0;
          Serial.println();
        }
      }
      bits = bits << 1 | bit;
      n--;
    }
    return bits;
  }
};

static LFSR16 LFSR = LFSR16();

// wrapper for LEDs
class LEDs {
private:
  uint n_leds = 4;
  // PINS - edit these values for the led pins
  std::array<uint, 4> pins = { 23, 32, 5, 2 };
public:
  void init() {
    for (uint i = 0; i < n_leds; i++) ledcAttach(pins[i], LED_FREQ, LED_BITS);
    all_off();
  }
  void set(uint led, uint level) {
    analogWrite(pins[led], level & 0xff);
  }
  void on(uint led) {
    set(led, 0xffu);
  }
  void on(uint led, bool on) {
    set(led, on ? 0xffu : 0x0u);
  }
  void off(uint led) {
    set(led, 0x0u);
  }
  void all_on() {
    for (uint i = 0; i < n_leds; i++) on(i);
  }
  void all_off() {
    for (uint i = 0; i < n_leds; i++) off(i);
  }
  void start_up() {
    all_on();
    delay(100);
    all_off();
    delay(100);
    all_on();
    delay(100);
    all_off();
  }
  void uint12(uint value, bool full) {
    for (uint i = 0; i < n_leds; i++) {
      set(i, min(value, MAX12) >> (full ? 4 : 7));
      if (value >= N10) value -= N10;
      else value = 0;
    }
  }
  void uint12r(uint value, bool full) {
    for (uint i = 0; i < n_leds; i++) {
      set(n_leds - 1 - i, min(value, MAX12) >> (full ? 4 : 7));
      if (value >= N10) value -= N10;
      else value = 0;
    }
  }
  void uint5(uint value, bool full) {
    all_off();
    if (value) set(value - 1, MAX12 >> (full ? 4 : 7));
  }
  void bin(uint value, bool full) {
    for (uint i = 0; i < n_leds; i++) {
      set(i, value & (1 << i) ? MAX12 >> (full ? 4 : 7) : 0);
    }
  }
  void uint_even(uint value, bool full) {
    on(n_leds-1, value > MAX11);
    value &= MAX11;
    uint third = N11 / 3;
    for (uint i = 0; i < n_leds-1; i++) {
      set(i, min(value, MAX12) >> (full ? 4 : 7));
      if (value >= third) value -= third;
      else value = 0;
    }
  }
};

// central location for coordinating (multiple) button presses, also wraps leds
class CentralState {
private:
  LEDs leds = LEDs();
public:
  uint button_mask = 0;
  uint button_mask_changed = false;
  uint n_buttons = 0;
  CentralState() = default;
  void init() {
    leds.init();
    leds.start_up();
  }
  void button(uint idx, bool on) {
    uint mask = 1 << idx;
    uint new_button_mask = button_mask;
    if (on) new_button_mask |= mask;
    else new_button_mask &= ~mask;
    button_mask_changed = new_button_mask != button_mask;
    button_mask = new_button_mask;
    n_buttons = std::popcount(button_mask);
    leds.all_off();
    // Serial.printf("button %d, on %d, button_mask %d, n_buttons %d\n", idx, on, button_mask, n_buttons);
  }
  void voice(uint idx, bool on) {
    if (!button_mask) leds.on(idx, on);
  }
  // todo - drop these (pot) once replaced by led calls - we don't care if it's a pot or not, so use led directly
  void pot(uint idx) {
    leds.on(idx);
  }
  void pot(uint idx, bool on) {
    leds.on(idx, on);
  }
  void pot_clear() {
    leds.all_off();
  }
  void led_10(uint value, bool full) {
    leds.uint12((value << 2) & 0xfff, full);
  }
  void led_11(uint value, bool full) {
    leds.uint12((value << 1) & 0xfff, full);
  }
  void led_11r(uint value, bool full) {
    leds.uint12r((value << 1) & 0xfff, full);
  }
  void led_12(uint value, bool full) {
    leds.uint12(value, full);
  }
  void led_5(uint value, bool full) {
    leds.uint5(value, full);
  }
  void led_bin(uint value, bool full) {
    leds.bin(value, full);
  }
  void led_even(uint value, bool full) {
    leds.uint_even(value, full);
  }
  void led_fm(uint value, bool full) {
    if (value < N11) led_11(value, full);
    else led_10(value, full);
  }
  void led(uint led, uint value, bool full) {
    leds.set(led, value >> (full ? 4 : 7));
  }
  void led_clear() {
    leds.all_off();
  }
  void led_centre(bool full) {
    led_clear();
    leds.on(1, 0xfff >> (full ? 4 : 7));
    leds.on(2, 0xfff >> (full ? 4 : 7));
  }
};

static CentralState STATE = CentralState();

class ButtonState {
private:
  uint button_mask;
public:
  ButtonState(uint button_mask) : button_mask(button_mask) {};
  bool selected = false;
  bool entry = false;
  bool exit = false;
  void update() {
    if (STATE.button_mask == button_mask) {
      if (!selected) {
        entry = true;
        selected = true;
        exit = false;
      } else {
        entry = false;
      }
    } else {
      if (selected) {
        entry = false;
        selected = false;
        exit = true;
      } else {
        exit = false;
      }
    }
  }
};

// quarter wave lookup (don't need to store complete sine wave; only first quarter)
class Quarter {
private:
  virtual uint lookup(uint phase);
public:
  Quarter() = default;
  int operator()(uint amp, int phase) {
    int sign = 1;
    if (phase >= N_SAMPLES / 2) {
      phase = N_SAMPLES - phase;
      sign = -1;
    };
    if (phase >= N_SAMPLES / 4) phase = (N_SAMPLES / 2) - phase;
    return sign * ((amp * lookup(phase)) >> 12);
  }
};

// implement sine as lookup
class Sine : public Quarter {
private:
  std::array<uint, 1 + N_SAMPLES / 4> table;
public:
  Sine() : Quarter() {
    for (uint i = 0; i < 1 + N_SAMPLES / 4; i++) table[i] = MAX12 * sin(2 * PI * i / N_SAMPLES);
  }
  uint lookup(uint phase) override {
    return table[phase];
  }
};

Sine SINE;

// implement square (no need for table, can just use constant)
class Square : public Quarter {
public:
  Square() : Quarter() {}
  uint lookup(uint phase) override {
    return MAX12;
  }
};

Square SQUARE;

class Triangle : public Quarter {
public:
  Triangle() : Quarter() {}
  uint lookup(uint phase) override {
    return (4 * MAX12 * phase) / N_SAMPLES;
  }
};

Triangle TRIANGLE;

class HiPass {
private:
  int prev_in = 0;
  int prev_out = 0;
  int alpha;
public:
  HiPass(int alpha) : alpha(alpha) {};
  int next(int in, int alpha2) {
    alpha = alpha2;
    return next(in);
  }
  int next(int in) {
    prev_out = (alpha * in - alpha * prev_in + (static_cast<int>(MAX12) - alpha) * prev_out) >> 12;
    prev_in = in;
    return prev_out;
  }
};

// generate the sound for each voice (which means tracking time and phase)
class Voice {
private:
  uint time = 0;
  int phase = 0;
  int fm_phase = 0;
  int noise[N_NOISE] = { 0 };
  uint noise_idx = 0;
public:
  uint idx;
  // parameters madified by UI
  volatile uint amp;   // 12 bits
  volatile uint freq;  // 12 bits
  volatile uint durn;  // 12 bits
  volatile uint fm;    // 12 bits
  volatile int shift;  // 12 bits signed
  Voice(uint idx, uint amp, uint freq, uint durn, uint fm, int shift) : idx(idx), amp(amp), freq(freq), durn(durn), fm(fm), shift(shift) {};
  void trigger() {
    time = 0;
    phase = 0;
  }
  int output_12() {
    uint durn_scaled = durn << 2;
    if (time == durn_scaled) STATE.voice(idx, false);
    if (time > durn_scaled) return 0;
    uint nv_fm = fm;
    uint waveform = nv_fm >> 10;
    uint fm_low10 = nv_fm & MAX10;
    uint fm_low11 = nv_fm & MAX11;
    uint nv_amp = amp;
    uint amp_11 = amp & MAX11;
    uint amp_scaled = series_exp(amp_11, 3) >> 15;
    uint linear_dec = MAX12 * (durn_scaled - time) / (durn_scaled + 1);
    uint quad_dec = series_exp(linear_dec, 2) >> 11;
    uint cube_dec = series_exp(linear_dec, 3) >> 17;
    uint hard = amp_scaled * (waveform == 2 ? cube_dec : quad_dec) >> 12;
    uint env_phase = (N_SAMPLES * time) / (1 + 2 * durn_scaled);
    uint soft = abs(SINE((amp_scaled * linear_dec) >> 11, env_phase));
    uint final_amp = nv_amp & N11 ? soft : hard;
    uint nv_freq = freq;
    // uint freq_scaled = 1 + (series_exp(nv_freq, 2) >> 12);
    // uint freq_scaled = 1 + (nv_freq >> 4) + ((nv_freq * nv_freq) >> 12);
    uint freq_scaled = 1 + nv_freq;
    if ((DBG_BEEP | DBG_CRASH | DBG_DRUM | DBG_MINIFM) && !idx && !random(DBG_LOTTERY))
      Serial.printf("amp_12 %d, amp_11 %d, amp_scaled %d, linear %d, quad %d, cube %d, hard %d, soft %d, amp %d, freq %d, freq_scaled %d, fm %d\n",
                    nv_amp & N11, amp_11, amp_scaled, linear_dec, quad_dec, cube_dec, hard, soft, final_amp, nv_freq, freq_scaled, nv_fm);
    int out = 0;
    switch (waveform) {
      case 0:
      case 1:
        out = drum(fm_low11, final_amp, freq_scaled, quad_dec, cube_dec);
        break;
      case 2:
        out = crash(fm_low10, final_amp, freq_scaled, linear_dec, quad_dec);
        break;
      case 3:
      default:
        out = minifm(N10 - fm_low10, final_amp, freq_scaled);
        break;
    }
    time++;
    return out;
  }
  int minifm(uint fm, uint final_amp, uint freq_scaled) {
    // hand tuned for squelchy noises
    if (DBG_TONE && !random(DBG_LOTTERY)) Serial.printf("%d fm %d\n", idx, fm);
    fm_phase += series_exp(fm << 1, 3) >> 22;
    while (fm_phase >= N_SAMPLES_EXTN) fm_phase -= N_SAMPLES_EXTN;
    phase += freq_scaled + SINE(max(1u, freq_scaled), fm_phase >> PHASE_EXTN);  // sic
    while (phase < 0) phase += N_SAMPLES_EXTN;
    while (phase >= N_SAMPLES_EXTN) phase -= N_SAMPLES_EXTN;
    int out = SINE(final_amp, phase >> PHASE_EXTN);
    if (DBG_MINIFM && !idx && !random(DBG_LOTTERY))
      Serial.printf("time %d, phase %d, out %d\n", time, phase, out);
    return out;
  }
  int crash(uint fm, uint final_amp, uint freq_scaled, uint linear_dec, uint quad_dec) {
    static HiPass hp(MAX11);
    fm_phase += (fm >> 2);
    while (fm_phase >= N_SAMPLES_EXTN) fm_phase -= N_SAMPLES_EXTN;
    phase += (freq_scaled << 1) + TRIANGLE(quad_dec, fm_phase >> PHASE_EXTN);
    while (phase < 0) phase += N_SAMPLES_EXTN;
    while (phase >= N_SAMPLES_EXTN) phase -= N_SAMPLES_EXTN;
    int out = SINE(MAX12, phase >> PHASE_EXTN);
    out += static_cast<int>(quad_dec >> 5) * (LFSR.next() ? 1 : -1);
    out += static_cast<int>(linear_dec >> 6) * (LFSR.next() ? 1 : -1);
    out = hp.next(out, quad_dec << 1);
    out = (out * static_cast<int>(final_amp)) >> 12;
    out = (out * static_cast<int>(final_amp)) >> 12;
    return compress(out, 3);
  }
  int drum(uint fm, uint final_amp, uint freq_scaled, uint quad_dec, uint cube_dec) {
    if (DBG_TONE && !random(DBG_LOTTERY)) Serial.printf("%d drum\n", idx);
    uint fmlo = ((fm & 0x3f) >> 2) + 1;  // bottom 6 bits
    freq_scaled >>= 4;
    // freq_scaled = freq_scaled >> 2;
    // if (fmlo == 1) freq_scaled = freq_scaled;
    phase += freq_scaled;
    phase += (quad_dec * fmlo) >> 10;
    while (phase < 0) phase += N_SAMPLES_EXTN;
    while (phase >= N_SAMPLES_EXTN) phase -= N_SAMPLES_EXTN;
    int out = SINE(final_amp, phase >> PHASE_EXTN);
    uint fmhi = (fm & 0x7c0) >> 6;  // top 5 bits
    out += static_cast<int>((final_amp * cube_dec * fmhi) >> 20) * (LFSR.next() ? 1 : -1);
    out += (fmhi > 16 && !(time & 0xf)) ? static_cast<int>((final_amp * quad_dec * fmhi) >> 19) * (LFSR.next() ? 1 : -1) : 0;
    if (DBG_DRUM && !idx && !random(DBG_LOTTERY))
      Serial.printf("time %d, fm %d, lo %d, hi %d, out %d\n", time, fm, fmlo, fmhi, out);
    return out;
  }
  void to(Voice& other) {
    other.amp = amp;
    other.freq = freq;
    other.durn = durn;
    other.fm = fm;
    other.shift = shift;
  }
  void to(VoiceData& other) {
    other.amp = amp;
    other.freq = freq;
    other.durn = durn;
    other.fm = fm;
    other.shift = shift;
  }
  void from(VoiceData& other) {
    amp = other.amp;
    freq = other.freq;
    durn = other.durn;
    fm = other.fm;
    shift = other.shift;
  }
};

std::array<Voice, 4> VOICES = {Voice(0, MAX10, 160, 1200,   0, 0),
                               Voice(1, MAX9,  240, 1000, 240, 0),
                               Voice(2, MAX9,  213, 1300, 150, 0),
                               Voice(3, MAX9,  320,  900, 240, 0)};

// standard euclidean pattern
class Euclidean {
private:
  uint n_places;
  uint n_beats;
  float frac_main;
  float n_main;
  uint prob;
  uint voice;
  uint current_place = 0;
  std::vector<uint> place_ref;       // beat index -> place index (reference)
  std::vector<uint> place_off;       // beat index -> place index (offset)
  std::vector<int> error;            // beat index -> error x MAX12
  std::vector<uint> index_by_error;  // error (abs(error) increasing) -> beat index
  std::vector<bool> is_main;         // beat index -> true if main (small error)
  std::vector<int> index_by_place;   // (offset) place index -> beat index (or -1)
  void trigger(uint delta) {
    uint n = voice + delta;
    if (gray(ENABLED) & 1 << n) {
      VOICES[n].trigger();
      STATE.voice(n, true);
    }
  }
public:
  // 1 <= n_beats <= n_places
  Euclidean(uint n_places, uint n_beats, float frac_main, uint prob, uint voice)
    : n_places(n_places), n_beats(n_beats), frac_main(max(0.0f, min(1.0f, frac_main))), prob(prob), voice(voice) {
    for (uint i = 0; i < n_beats; i++) {
      float x = n_places * i / static_cast<float>(n_beats);
      place_ref.push_back(round(x));
      place_off.push_back(round(x));
      error.push_back(static_cast<int>(MAX12 * (x - round(x))));
    }
    uint regular[n_beats] = {0};
    int penalty = MAX11;
    for (uint i = 0; i < n_beats; i++) {
      if (error[i]) penalty = min(penalty, abs(error[i]));
    }
    penalty /= 2;
    // should penalty be +ve or -ve?
    if (random(2)) penalty = -1 * penalty;
    for (uint i = 2; i < n_beats; i++) {
      if (!(n_beats % i) && !(n_places % i)) {
        for (uint j = 0; j < n_beats; j += i) regular[j] = penalty;
        penalty /= 2;
      }
    }
    for (uint i = 0; i < n_beats; i++) {
      if (!error[i]) error[i] = regular[i];
    }
    index_by_error.resize(n_beats, 0);
    std::iota(index_by_error.begin(), index_by_error.end(), 0);
    std::sort(index_by_error.begin(), index_by_error.end(), [this](int i, int j) {
      return abs(this->error[i]) < abs(this->error[j]);
    });
    // for (uint i = 0; i < n_beats; i++) Serial.printf("%d %f\n", i, error[index_by_error[i]]);
    n_main = max(0u, min(n_beats, static_cast<uint>(round(n_beats * frac_main))));
    is_main.resize(n_beats, false);
    for (uint i = 0; i < n_main; i++) is_main[index_by_error[i]] = true;
    index_by_place.resize(n_places, -1);
    for (uint i = 0; i < n_beats; i++) index_by_place[place_off[i]] = i;
    // for (uint i = 0; i < n_beats; i++) Serial.printf("%d: %d\n", i, place_ref[i]);
  };
  Euclidean() : Euclidean(2, 1, 0.5, MAX12, 0) {};  // used only as temp value in arrays
  bool operator==(const Euclidean other) {
    return other.n_places == n_places && other.n_beats == n_beats && other.n_main == n_main && other.prob == prob && other.voice == voice;
  }
  void jiggle() {
    // error is signed to MAX11 (MAX12 * 0.5) while prob is unisgned MAX12
    for (uint i = 0; i < n_beats - n_main; i++) {
      uint frac_m12 = (prob * static_cast<uint>(abs(error[i]))) >> 11;
      if (random(MAX12) < frac_m12) {
        if (place_ref[i] == place_off[i]) {
          if (error[i] > 0 && index_by_place[place_off[i] + 1] == -1) {
            place_off[i]++;
            index_by_place[place_off[i]] = i;
            index_by_place[place_ref[i]] = -1;
            if (DBG_JIGGLE) Serial.printf("%d: %d -> %d\n", voice, place_ref[i], place_off[i]);
          } else if (error[i] < 0 && index_by_place[place_off[i] - 1] == -1) {
            place_off[i]--;
            index_by_place[place_off[i]] = i;
            index_by_place[place_ref[i]] = -1;
            if (DBG_JIGGLE) Serial.printf("%d: %d -> %d\n", voice, place_ref[i], place_off[i]);
          }
        } else {
          if (error[i] > 0 && index_by_place[place_ref[i]] == -1) {
            index_by_place[place_off[i]] = -1;
            place_off[i]--;
            index_by_place[place_ref[i]] = i;
            if (DBG_JIGGLE) Serial.printf("%d: %d -> %d\n", voice, place_ref[i], place_off[i]);
          } else if (error[i] < 0 && index_by_place[place_ref[i]] == -1) {
            index_by_place[place_off[i]] = -1;
            place_off[i]++;
            index_by_place[place_ref[i]] = i;
            if (DBG_JIGGLE) Serial.printf("%d: %d -> %d\n", voice, place_ref[i], place_off[i]);
          }
        }
        return;
      }
    }
  }
  void on_beat(bool main) {
    static uint nl = 0;
    if (!nl && DBG_PATTERN) Serial.printf("\n");
    nl = (nl + 1) & 0xff;
    int beat = index_by_place[current_place];
    if (beat != -1 && is_main[beat] == main) {
      trigger(main ? 0 : 1);
      // Serial.printf("on_beat: place %d, beat %d, voice %d\n", current_place, beat, voice + (main ? 0 : 1));
      if (DBG_PATTERN) Serial.printf("%d", voice + (main ? 0 : 1));
    } else if (DBG_PATTERN) Serial.printf("-");
    if (!main) {  // main is done before minor
      if (++current_place == n_places) current_place = 0;
    }
  }
};

// exponential moving average used for smoothing time series
template<typename T> class EMA {
private:
  uint bits;
  uint denom;
  uint xtra;
  T state;
protected:
  virtual T get_state() {
    return state;
  }
  virtual void set_state(T s) {
    state = s;
  }
public:
  uint num;
  EMA(uint bits, uint num, uint xtra, T state)
    : bits(bits), denom(1 << bits), xtra(xtra), state(state << xtra), num(num){};
  T next(T val) {
    set_state((get_state() * static_cast<T>(denom - num) + (val << xtra) * static_cast<T>(num)) >> bits);
    return read();
  }
  T read() {
    return get_state() >> xtra;
  }
};

// encapsulate a pot position and associated (smoothed) state
class Pot {
private:
  static const uint ema_xbits = 3;
  uint pin;
  EMA<uint> ema = EMA<uint>(3, 1, 3, 0);
public:
  uint state;
  Pot(uint pin)
    : pin(pin){};
  void init() {
    pinMode(pin, INPUT);
  }
  void read_state() {
    state = MAX12 - ema.next(analogRead(pin)) - 1;
  }  // inverted and hack to zero
};

// all the pots
// PINS - edit these values for the potentiometer pins
std::array<Pot, 4> POTS = {Pot(13), Pot(14), Pot(27), Pot(12)};

// debounce buttons
// subclass to provide actions (although i think there's only one subclass...)
class Button {
private:
  static const uint debounce = 50;
  uint pin;
  bool tmp_pressed = false;
  uint tmp_change = 0;
  virtual void take_action() = 0;
protected:
  uint idx;
  bool pressed = false;
  bool changed = false;
public:
  Button(uint idx, uint pin) : pin(pin), idx(idx){};
  void init() {
    pinMode(pin, INPUT_PULLUP);
  }
  void read_state() {
    changed = false;
    bool current = !digitalRead(pin);  // inverted
    uint now = millis();
    if (tmp_change == 0 || current != tmp_pressed) {
      // fluctuating, save new state
      tmp_pressed = current;
      tmp_change = now;
    } else if (now - tmp_change > debounce && tmp_pressed != pressed) {
      // changed and steady
      pressed = tmp_pressed;
      changed = true;
      STATE.button(idx, pressed);
    }
    take_action();
  }
};

// assorted ways to apply changes from a pot to some underlying parameter (implements the catch-up mechanism)
class PotsReader {
private:
  static const uint thresh = 10;
  std::array<bool, 4> enabled = {false, false, false, false};
  std::array<uint, 4> posn = {N12, N12, N12, N12};
  int active = -1;
  const std::array<uint, 4> prime = {2, 3, 5, 7};
  bool check_enabled(uint value, uint pot) {
    if (posn[pot] == N12) posn[pot] = POTS[pot].state;
    else if (abs(static_cast<int>(posn[pot]) - static_cast<int>(POTS[pot].state)) > thresh) {
      posn[pot] = POTS[pot].state;
      if (active != pot) {
        STATE.led_clear();
        active = pot;
      }
    }
    if (!enabled[pot] && abs(static_cast<int>(value) - static_cast<int>(POTS[pot].state)) < thresh) enabled[pot] = true;
    return enabled[pot];
  }
protected:
  void disable() {
    std::fill(std::begin(enabled), std::end(enabled), false);
    std::fill(std::begin(posn), std::end(posn), N12);
    active = -1;
  }
  void update_11(volatile uint* destn, uint pot) {
    if (check_enabled(*destn, pot)) *destn = POTS[pot].state;
    if (pot == active) STATE.led_11(*destn, enabled[pot]);
  }
  void update_12(volatile uint* destn, uint pot) {
    if (check_enabled(*destn, pot)) *destn = POTS[pot].state;
    if (pot == active) STATE.led_12(*destn, enabled[pot]);
  }
  void update_5(uint* destn, uint pot) {
    uint target = (MAX12 * *destn) / 5;
    if (check_enabled(target, pot)) *destn = (POTS[pot].state * 5) / N12;  // 0-4
    if (pot == active) STATE.led_5((target * 5) / MAX12, enabled[pot]);
  }
  void update_bin(uint* destn, uint pot) {
    uint target = *destn << 8;
    if (check_enabled(target, pot)) *destn = POTS[pot].state >> 8;
    if (pot == active) STATE.led_bin(target >> 8, enabled[pot]);
  }
  void update_fm(volatile uint* destn, uint pot) {
    if (check_enabled(*destn, pot)) *destn = POTS[pot].state;
    if (pot == active) STATE.led_fm(*destn, enabled[pot]);
  }
  void update_12(volatile uint* destn, uint zero, int bits, uint pot) {
    int target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (check_enabled(target, pot)) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
    if (pot == active) STATE.led_12(target, enabled[pot]);
  }
  void update_12(float* destn, uint pot) {
    static const int EDGE = 4;  // guarantee 0-1 float full range
    if (check_enabled(*destn * MAX12, pot))
      *destn = max(0.0f, min(1.0f, static_cast<float>((static_cast<int>(POTS[pot].state) - EDGE)) / (static_cast<int>(MAX12) - 2 * EDGE)));
    if (pot == active) STATE.led_12(*destn * MAX12, pot);
  }
  void update_even(uint* destn, uint pot) {
    if (check_enabled(*destn, pot)) *destn = POTS[pot].state;
    if (pot == active) STATE.led_even(*destn, enabled[pot]);
  }
  void update_lr(float* destn, uint pot, bool p1) {
    static const int EDGE = 4;  // guarantee 0-1 float full range
    if (check_enabled(*destn * MAX12, pot))
      *destn = max(0.0f, min(1.0f, static_cast<float>((static_cast<int>(POTS[pot].state) - EDGE)) / (static_cast<int>(MAX12) - 2 * EDGE)));
    if (pot == active) {
      STATE.led(p1 ? 0 : 2, *destn * MAX12, enabled[pot]);
      STATE.led(p1 ? 1 : 3, (1 - *destn) * MAX12, enabled[pot]);
    }
  }
  void update_prime(volatile uint* destn, uint zero, int bits, uint pot) {
    int target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (check_enabled(target, pot)) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
    if (pot == active) {
      for (uint i = 0; i < 4; i++) STATE.pot(i, !(*destn % prime[i]));
    }
  }
  void update_prime(float* destn, uint scale, uint pot) {
    static const int EDGE = 4;  // guarantee 0-1 float full range
    if (check_enabled(*destn * MAX12, pot))
      *destn = max(0.0f, min(1.0f, static_cast<float>((static_cast<int>(POTS[pot].state) - EDGE)) / (static_cast<int>(MAX12) - 2 * EDGE)));
    if (pot == active) {
      for (uint i = 0; i < 4; i++) STATE.pot(i, !(static_cast<uint>(scale * *destn) % prime[i]));
    }
  }
  void update_subdiv(volatile uint* destn, uint zero, int bits, uint pot) {
    int target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (check_enabled(target, pot)) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
    if (pot == active) {
      for (uint i = 0; i < 4; i++) STATE.pot(i, !(SUBDIVS[*destn] % prime[i]));
    }
  }
  void update_gray(volatile uint* destn, int zero, int bits, uint pot) {
    uint mask = (1 << bits) - 1;
    uint target = ((static_cast<int>(*destn) - zero) & mask) << (12 - bits);
    if (check_enabled(target, pot)) *destn = (zero + (POTS[pot].state >> (12 - bits))) & mask;
    if (pot == active) {
      uint g = gray(*destn);
      for (uint i = 0; i < 4; i++) STATE.pot(i, g & (1 << i));
    }
  }
  void update_signed(volatile int* destn, uint pot) {
    int target = *destn + MAX11;
    if (check_enabled(target, pot)) *destn = POTS[pot].state - MAX11;
    if (pot == active) {
      if (abs(*destn) < 16) STATE.led_centre(enabled[pot]);
      else if (*destn >= 0) STATE.led_11(MAX11 - *destn, enabled[pot]);
      else STATE.led_11r(MAX11 + *destn, enabled[pot]);
    }
    // if (enabled[pot] && !random(DBG_LOTTERY/10)) Serial.printf("%d\n", *destn);
  }
};

uint gray(uint n) {
  return n ^ (n >> 1);
}

// subclass button to edit voice parameters
class VoiceButton : public Button, public PotsReader {
private:
  bool editing = false;
  Voice& voice;
  void take_action() {
    if (pressed && STATE.n_buttons == 1) {
      editing = true;
      update_11(&voice.amp, 0);
      update_12(&voice.freq, 1);
      update_12(&voice.durn, 2);
      update_fm(&voice.fm, 3);
      if (DBG_VOICE) Serial.printf("a %d, f %d, d %d, n %d\n", voice.amp, voice.freq, voice.durn, voice.fm);
    } else {
      if (editing) {
        // Serial.printf("Voice(%d, %d, %d, %d, %d)\n", idx, voice.amp, voice.freq, voice.durn, voice.fm);
        editing = false;
        STATE.led_clear();
      }
      disable();
    }
  }
public:
  VoiceButton(uint idx, uint pin, Voice& voice) : Button(idx, pin), PotsReader(), voice(voice){};
};

// PINS - edit these values for the button pins
std::array<VoiceButton, 4> VOICE_BUTTONS = {VoiceButton(0, 18, VOICES[0]),
                                            VoiceButton(1,  4, VOICES[1]),
                                            VoiceButton(2, 15, VOICES[2]),
                                            VoiceButton(3, 19, VOICES[3])};

// global parameters - hold down middle two buttons
class GlobalButtons : public PotsReader {
public:
  GlobalButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0x6) {
      update_12(&BPM, 30, -2, 0);
      update_subdiv(&SUBDIV_IDX, 0, -8, 1);
    } else {
      disable();
    }
  }
};

static GlobalButtons GLOBAL_BUTTONS = GlobalButtons();

// handle passing of complex state between threads (everything else is a volatile uint, i think, but the Euclidean class is complex)
// this stores three things:
// - a set of values to create "the next" euclidean object while editing is in progress
// - a "next" euclidean object created when editing last exited
// - a "current" euclidean object that is used for sound generation
// it is responsible for:
// - creating of the "next" instance when editing finishes
// - moving "next" to "current" when a new cycle begins (avoiding copy/assignment)
// - holding the "current" instance in memory while it is in use
// - avoiding access conflicts
class EuclideanVault {
private:
  std::mutex access;
  Euclidean euclideans[2];
  bool updated = false;
  uint current = 0;
  uint next = 1;
public:
  uint voice;
  uint n_places;
  float frac_beats;
  float frac_main;
  uint prob;
  EuclideanVault(uint n_places, float frac_beats, float frac_main, uint prob, uint voice)
    : voice(voice), n_places(n_places), frac_beats(frac_beats), frac_main(frac_main), prob(prob) {
    euclideans[0] = Euclidean(n_places, max(1u, static_cast<uint>(n_places * frac_beats)), frac_main, prob, voice);
    euclideans[1] = Euclidean(n_places, max(1u, static_cast<uint>(n_places * frac_beats)), frac_main, prob, voice);
  }
  void apply_edit(bool dump) {
    std::lock_guard<std::mutex> lock(access);
    uint n_beats = max(1u, static_cast<uint>(n_places * frac_beats));
    Euclidean candidate = Euclidean(n_places, n_beats, frac_main, prob, voice);
    if (euclideans[updated ? next : current] != candidate) {
      if (DBG_EUCLIDEAN) Serial.printf("Euclidean(%d, %d, %.3f, %d, %d)\n", n_places, n_beats, frac_main, prob, voice);
      euclideans[next] = candidate;
      updated = true;
    }
  };
  Euclidean* get() {
    std::unique_lock<std::mutex> lock(access, std::defer_lock);
    if (lock.try_lock() && updated) {
      std::swap(current, next);
      updated = false;
    }
    return &euclideans[current];
  }
};

static EuclideanVault VAULTS[2] = {EuclideanVault(16, 0.666, 0.5, 0, 0), EuclideanVault(25, 0.666, 0.5, MAX11, 2)};

// edit patterns - hold down left or right two buttons (mask)
class EuclideanButtons : public PotsReader {
private:
  uint mask;
  EuclideanVault& vault;
  bool editing = false;
public:
  EuclideanButtons(uint mask, EuclideanVault& vault) : PotsReader(), mask(mask), vault(vault) {};
  void read_state() {
    if (STATE.button_mask == mask) {
      editing = true;
      update_prime(&vault.n_places, 2, -7, 0);
      update_prime(&vault.frac_beats, vault.n_places, 1);
      update_lr(&vault.frac_main, 2, mask == 0x3u);
      update_12(&vault.prob, 3);
      vault.apply_edit(false);
    } else if (editing) {
      vault.apply_edit(true);
      disable();
      editing = false;
    }
  }
};

static EuclideanButtons EUCLIDEAN_BUTTONS_LEFT = EuclideanButtons(0x3u, VAULTS[0]);
static EuclideanButtons EUCLIDEAN_BUTTONS_RIGHT = EuclideanButtons(0xcu, VAULTS[1]);

// reverb via array of values (could maybe save space with int16, but store extra bits to reduce noise)
template<int BITS> class Reverb {
private:
  static const uint max_size = 1 << BITS;
  int tape[max_size];
  uint write = 0;

  class TapeEMA : public EMA<int> {
  private:
    Reverb<BITS>* reverb;
  protected:
    int get_state() {
      return reverb->tape[reverb->write];
    }
    void set_state(int val) {
      reverb->tape[reverb->write] = val;
    }
  public:
    TapeEMA(Reverb<BITS>* reverb, uint bits, uint num, uint xtra) : EMA<int>(bits, num, xtra, 0), reverb(reverb) {};
  };

public:
  uint size = max_size;
  TapeEMA head;
  Reverb() : head(TapeEMA(this, 12, N12, 4)) {};  // by default disabled
  int next(int val) {
    write = (write + 1) % max(1u, size);  // can't be "& mask" because size can vary
    return head.next(val);
  }
};

static Reverb REVERB = Reverb<13>();

// post-process - outer two buttons
class PostButtons : public PotsReader {
private:
  uint buffer_frac = ((LOCAL_BUFFER_SIZE * MAX11) / MAX_LOCAL_BUFFER_SIZE) + (LOCAL_BUFFER_SIZE & 0x1 ? 0 : N11);
public:
  PostButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0x9) {
      update_12(&REVERB.size, 0, 1, 0);
      update_12(&REVERB.head.num, 1, 0, 1);
      update_12(&COMP_BITS, 0, -9, 2);
      float prev = buffer_frac;
      update_even(&buffer_frac, 3);
      if (buffer_frac != prev) {
        // frac is 12 bits; msb is even flag
        LOCAL_BUFFER_SIZE = max(MIN_LOCAL_BUFFER_SIZE, min(MAX_LOCAL_BUFFER_SIZE, (buffer_frac & MAX11) * MAX_LOCAL_BUFFER_SIZE / MAX11));
        if (buffer_frac > MAX11) {
          if (LOCAL_BUFFER_SIZE & 0x1) {
            LOCAL_BUFFER_SIZE += 1;
            if (LOCAL_BUFFER_SIZE > MAX_LOCAL_BUFFER_SIZE) LOCAL_BUFFER_SIZE -= 2;
          }
        } else {
          if (!(LOCAL_BUFFER_SIZE & 0x1)) {
            LOCAL_BUFFER_SIZE += 1;
            if (LOCAL_BUFFER_SIZE > MAX_LOCAL_BUFFER_SIZE) LOCAL_BUFFER_SIZE -= 2;
          }
        }
        REFRESH_US = (1000000 * LOCAL_BUFFER_SIZE) / SAMPLE_RATE_HZ;
      }
    } else {
      disable();
    }
  }
};

static PostButtons POST_BUTTONS = PostButtons();

// performance controls - inner two buttoms plus left
class PerfButtons : public PotsReader {
private:
  uint enabled = ENABLED;  // not applied until buttons released
public:
  PerfButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0x7) {
      update_gray(&enabled, 11, 4, 0);
      update_signed(&VOICES[1].shift, 1);
      update_signed(&VOICES[2].shift, 2);
      update_signed(&VOICES[3].shift, 3);
    } else {
      ENABLED = enabled;
      disable();
    }
  }
};

static PerfButtons PERFORMANCE_BUTTONS = PerfButtons();

class Config {
private:
  Preferences prefs;
  AllData build() {
    Serial.printf("build\n");
    AllData current;
    for (uint i = 0; i < 4; i++) VOICES[i].to(current.voices[i]);
    for (uint i = 0; i < 2; i++) {
      current.vaults[i].n_places = VAULTS[i].n_places;
      current.vaults[i].frac_beats = VAULTS[i].frac_beats;
      current.vaults[i].frac_main = VAULTS[i].frac_main;
      current.vaults[i].prob = VAULTS[i].prob;
      current.vaults[i].voice = VAULTS[i].voice;
    }
    current.reverb_size = REVERB.size;
    current.reverb_head_num = REVERB.head.num;
    current.bpm = BPM;
    current.subdiv_idx = SUBDIV_IDX;
    current.comp_bits = COMP_BITS;
    current.local_buffer_size = LOCAL_BUFFER_SIZE;
    return current;
  }
  void apply(AllData data) {
    Serial.printf("apply\n");
    for (uint i = 0; i < 4; i++) VOICES[i].from(data.voices[i]);
    for (uint i = 0; i < 2; i++) {
      VAULTS[i].n_places = data.vaults[i].n_places;
      VAULTS[i].frac_beats = data.vaults[i].frac_beats;
      VAULTS[i].frac_main = data.vaults[i].frac_main;
      VAULTS[i].prob = data.vaults[i].prob;
      VAULTS[i].voice = data.vaults[i].voice;
    }
    REVERB.size = data.reverb_size;
    REVERB.head.num = data.reverb_head_num;
    BPM = data.bpm;
    SUBDIV_IDX = data.subdiv_idx;
    COMP_BITS = data.comp_bits;
    LOCAL_BUFFER_SIZE = data.local_buffer_size;
  }
  bool exists(uint idx) {
    char* key = new_key(idx);
    bool exists = prefs.isKey(key);
    delete[] key;
    Serial.printf("exists %d %d\n", idx, exists);
    return exists;
  }
  void remove(uint idx) {
    Serial.printf("remove %d\n", idx);
    if (exists(idx)) {
      char *key = new_key(idx);
      prefs.remove(key);
      delete[] key;
    }
  }
  AllData really_read(uint idx) {
    Serial.printf("really_read %d\n", idx);
    byte tmp[sizeof(AllData)];
    char *key = new_key(idx);
    prefs.getBytes(key, tmp, sizeof(AllData));
    AllData data;
    memcpy(&data, tmp, sizeof(AllData));
    delete[] key;
    return data;
  }
  void really_save(AllData data, uint idx) {
    Serial.printf("really_save %d\n", idx);
    byte tmp[sizeof(AllData)];
    memcpy(tmp, &data, sizeof(AllData));
    char *key = new_key(idx);
    uint len = prefs.putBytes(key, tmp, sizeof(AllData));
    Serial.printf("wrote %d/%d bytes\n", len, sizeof(AllData));
    delete[] key;
  }
  char* new_key(uint idx) {
    char *key = new char[2];
    sprintf(key, "%x", idx);
    Serial.printf("key %d %s\n", idx, key);
    return key;
  }
public:
  Config() {
    bool ok = prefs.begin("xqria", false);
    Serial.printf("prefs opened %d\n", ok);
  }
  ~Config() {
    prefs.end();
    Serial.printf("prefs closed\n");
  }
  void save(uint idx) {
    Serial.printf("save %d\n", idx);
    if (idx > 0 && idx < 15) {
      if (exists(idx)) {
        AllData data = really_read(idx);
        if (exists(15)) remove(15);
        really_save(data, 15);  // TODO - 15 constant
      }
      AllData current = build();
      really_save(current, idx);
      exists(idx);
    }
  }
  void read(uint idx) {
    Serial.printf("read %d\n", idx);
    if (idx > 0 && idx < 16) {
      if (exists(idx)) {
        AllData data = really_read(idx);
        if (exists(15)) remove(15);
        AllData current = build();
        really_save(current, 15);
        apply(data);
      }
    }
  }
};

class EditButtons : public PotsReader {
private:
  ButtonState state = ButtonState(0xe);
  uint source = 0;
  uint destn = 0;
  uint save = 0;
  uint read = 0;
public:
  EditButtons() = default;
  void read_state() {
    state.update();
    if (state.selected) {
      update_5(&source, 0);
      update_gray(&destn, 0, 4, 1);
      update_bin(&read, 2);
      update_bin(&save, 3);
    }
    if (state.exit) {
      Config config;
      if (source > 0 && source < 5 && destn) {
        source--;  // 0 is none; 1-4 number the sources, but we want zero indexing
        destn = gray(destn);
        for (uint i = 0; i < 4; i++) {
          if (source != i && (destn & 1 << i)) {
            if (DBG_COPY) Serial.printf("copying %d -> %d\n", source, i);
            VOICES[source].to(VOICES[i]);
          }
        }
      } else if (save > 0 && save < 16) {
        config.save(save);
      } else if (read > 0 && read < 16) {
        config.read(read);
      }
      // reset
      source = 0;
      destn = 0;
      read = 0;
      save = 0;
      disable();
    }
  }
};

static EditButtons EDIT_BUTTONS = EditButtons();

// use a timer to give regular sampling and (hopefully) reduce noise
SemaphoreHandle_t timer_semaphore;
esp_timer_handle_t timer_handle;

void IRAM_ATTR timer_callback(void*) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(timer_semaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

int compress(int out, uint bits) {
  int sign = sgn(out);
  uint absolute = abs(out);
  for (uint i = (8 - bits); i < 8; i++) {
    uint limit = 1 << i;
    if (absolute > limit) absolute = limit + (absolute - limit) / 2;
  }
  return sign * static_cast<int>(absolute);
}

// apply post-processing
uint post_process(int vol) {
  // apply compressor
  int soft_clipped = compress(vol, COMP_BITS);
  // apply reverb
  int reverbed = REVERB.next(soft_clipped);
  // int reverbed = soft_clipped;
  if (DBG_REVERB && !random(DBG_LOTTERY)) Serial.printf("reverb %d -> %d\n", soft_clipped, reverbed);
  // hard clip
  int offset = reverbed + MAX7;
  int hard_clipped = max(0, min(static_cast<int>(MAX8), offset));
  if (DBG_VOLUME && !random(DBG_LOTTERY))
    Serial.printf("comp %d; vol %d; soft %d; hard %d\n",
                  8 - COMP_BITS, vol, soft_clipped, hard_clipped);
  return hard_clipped;
}

class Trigger {
private:
  EuclideanVault& vault;
  Voice& voice;
  bool major;
  bool subdiv;
  Euclidean* rhythm = nullptr;
  enum Phase {Idle, Jiggle, Update};
  Phase phase = Idle;
  uint trigger = 0;
  void recalculate(int64_t ticks) {
    rhythm = vault.get();
    uint nv_interval = BEAT_SCALE / BPM;
    if (subdiv) nv_interval = (nv_interval * SUBDIVS[SUBDIV_IDX]) / 60;
    uint beat = 1 + (ticks / nv_interval);
    trigger = nv_interval * beat;
    // Serial.printf("reacalculate: voice %d major %d trigger %d\n", voice.idx, major, trigger);
  }
public:
  Trigger(EuclideanVault& vault, Voice& voice, bool major, bool subdiv) : vault(vault), voice(voice), major(major), subdiv(subdiv) {
    recalculate(0);
  }
  void on(int64_t ticks) {  // try to spread work across multiple ticks
    ticks += voice.shift / static_cast<int>(BPM * SUBDIVS[SUBDIV_IDX] / 60);
    if (phase == Idle && ticks > trigger) {
      rhythm->on_beat(major);
      phase = Jiggle;
    } else if (phase == Jiggle) {
      rhythm->jiggle();
      phase = Update;
    } else if (phase == Update) {
      recalculate(ticks);
      phase = Idle;
    }
  }
};

class Audio {
private:
  int64_t ticks = 0;
  unsigned long start = 0;
  Trigger triggers[4];
public:
  Audio(Trigger trigger1, Trigger trigger2, Trigger trigger3, Trigger trigger4) : triggers(trigger1, trigger2, trigger3, trigger4) {};
  void generate(uint n, uint8_t data[]) {
    for (uint i = 0; i < n; i++) {
      uint j = ticks & 0x3;  // really subtle fix - we can get rhythms switching on the same beat if these don't match
      triggers[j].on(ticks-j);
      ticks++;
      int vol = 0;
      for (Voice& voice : VOICES) vol += voice.output_12();
      data[i] = post_process(vol >> 4);
    }
  }
};

// run the ui on other core
static TaskHandle_t ui_handle = NULL;

// main loop on other core for slow/ui operations
void ui_loop(void*) {
  while (1) {
    for (Pot& p : POTS) p.read_state();
    for (Button& b : VOICE_BUTTONS) b.read_state();
    GLOBAL_BUTTONS.read_state();
    EUCLIDEAN_BUTTONS_LEFT.read_state();
    EUCLIDEAN_BUTTONS_RIGHT.read_state();
    POST_BUTTONS.read_state();
    PERFORMANCE_BUTTONS.read_state();
    EDIT_BUTTONS.read_state();
    vTaskDelay(1);
  }
}

static Audio AUDIO(Trigger(VAULTS[0], VOICES[0], true, false), 
                   Trigger(VAULTS[0], VOICES[1], false, false), 
                   Trigger(VAULTS[1], VOICES[2], true, true),
                   Trigger(VAULTS[1], VOICES[3], false, true));
static uint8_t BUFFER[MAX_LOCAL_BUFFER_SIZE];
static SemaphoreHandle_t dma_semaphore;
static BaseType_t dma_flag = pdFALSE;
static dac_continuous_handle_t dac_handle;
static EMA<uint> DMA_INTERVAL = EMA<uint>(4, 1, 3, 0);
static uint DMA_ERRORS = 0;

// fast copy of existing data into buffer
static IRAM_ATTR bool dac_callback(dac_continuous_handle_t handle, const dac_event_data_t* event, void* user_data) {
  unsigned long start = micros();
  uint loaded = 0;
  dac_continuous_write_asynchronously(handle, static_cast<uint8_t*>(event->buf), event->buf_size, BUFFER, LOCAL_BUFFER_SIZE, &loaded);
  if (loaded != LOCAL_BUFFER_SIZE) DMA_ERRORS += 1;
  if (event->buf_size != DMA_BUFFER_SIZE) DMA_ERRORS += 1;
  if (event->write_bytes != 2 * LOCAL_BUFFER_SIZE) DMA_ERRORS += 1;
  xSemaphoreGiveFromISR(dma_semaphore, &dma_flag);  // flag refill
  DMA_INTERVAL.next(micros() - start);
  return false;
}

// slow fill of buffer
void update_buffer() {
  static EMA<uint> avg = EMA<uint>(4, 1, 3, REFRESH_US);
  unsigned long start = micros();
  AUDIO.generate(LOCAL_BUFFER_SIZE, BUFFER);
  uint durn = micros() - start;
  uint avg_durn = avg.next(durn);
  uint dma_durn = DMA_INTERVAL.read();
  if (DBG_TIMING && !random(DBG_LOTTERY / max(1u, LOCAL_BUFFER_SIZE >> 3)))
    Serial.printf("buffer %d filled in %dus (avg %dus, free %dus, duty %.1f%%); dma loaded in %dus; total %dus, duty %.1f%%; errors %d %s\n",
                  LOCAL_BUFFER_SIZE, durn, avg_durn, REFRESH_US - avg_durn, 100 * avg_durn / static_cast<float>(REFRESH_US),
                  dma_durn, dma_durn + avg_durn, 100 * (dma_durn + avg_durn) / static_cast<float>(REFRESH_US), 
                  DMA_ERRORS, DMA_ERRORS ? "XXXX" : "");
}

// refill buffer when flagged
void loop() {
  for (;;) {
    if (xSemaphoreTake(dma_semaphore, portMAX_DELAY) == pdTRUE) update_buffer();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("hello world");
  for (Button& b : VOICE_BUTTONS) b.init();
  for (Pot& p : POTS) p.init();
  STATE.init();

  xTaskCreatePinnedToCore(&ui_loop, "UI Loop", 10000, NULL, 1, &ui_handle, 0);

  update_buffer();  // first callback happens immediately
  dma_semaphore = xSemaphoreCreateBinary();
  dac_continuous_config_t dac_config = {
    .chan_mask = DAC_CHANNEL_MASK_CH0,
    .desc_num = 8,  // 2 allows pinpong for large buffers, but a larger value seems to help small buffers
    .buf_size = DMA_BUFFER_SIZE,
    .freq_hz = SAMPLE_RATE_HZ,
    .offset = 0,
    .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
    .chan_mode = DAC_CHANNEL_MODE_SIMUL  // not used for single channel
  };
  dac_continuous_new_channels(&dac_config, &dac_handle);
  dac_continuous_enable(dac_handle);
  dac_event_callbacks_t callbacks = {
    .on_convert_done = dac_callback,
    .on_stop = nullptr
  };
  dac_continuous_register_event_callback(dac_handle, &callbacks, nullptr);
  dac_continuous_start_async_writing(dac_handle);
};
