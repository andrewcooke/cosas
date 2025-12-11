
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
const uint INTERNAL_BITS = 16;
const uint INTERNAL_N = 1 << INTERNAL_BITS;
const uint INTERNAL_MAX = INTERNAL_N - 1;
const uint POT_BITS = 12;
const uint POT_N = 1 << POT_BITS;
const uint POT_MAX = POT_N - 1;
const uint DAC_BITS = 8;
const uint OVERSAMPLE_BITS = 1;  // currently can't quite suppoer 2 bits - get underruns when all voices active
const uint TAU_BITS = INTERNAL_BITS + 1 + OVERSAMPLE_BITS;
const uint TAU_N = 1 << TAU_BITS;
const uint TAU_MAX = TAU_N - 1;
const uint TABLE_BITS = 12;
const uint TABLE_N = 1 << TABLE_BITS;
const uint BEAT_SCALE = (SAMPLE_RATE_HZ << OVERSAMPLE_BITS) * 60;  // ticks for 1 bpm
const std::array<uint, 16> SUBDIVS = {5, 10, 12, 15, 20, 24, 25, 30, 35, 36, 40, 45, 48, 50, 55, 60};  // by luck length is power of 2
const uint MAX_COMP_BITS = 12;

// used in crash sound
const uint NOISE_BITS = 6;
const uint N_NOISE = 1 << NOISE_BITS;
const uint NOISE_MASK = N_NOISE - 1;

const uint LED_FREQ = 1000;
const uint LED_BITS = 8;
const uint LED_MAX = (1 << LED_BITS) - 1;
const uint LED_DIM_BITS = 3;

// global parameters that can be changed during use
//volatile static uint BPM = 90;
volatile static uint BPM = 200;
volatile static uint SUBDIV_IDX = 15;
volatile static uint COMP_BITS = 0;
volatile static uint SHIFT_BITS = 0;
// volatile static uint ENABLED = 10;  // all enabled (gray(10) = 9xf)
volatile static uint ENABLED = 1;
volatile static uint LOCAL_BUFFER_SIZE = MAX_LOCAL_BUFFER_SIZE;  // has to be even
volatile static uint REFRESH_US = (1000000 * LOCAL_BUFFER_SIZE) / (SAMPLE_RATE_HZ << OVERSAMPLE_BITS);

const uint DBG_LOTTERY = 10000;
const bool DBG_VOICE = false;
const bool DBG_LFSR = false;
const bool DBG_VOLUME = false;
const bool DBG_COMP = false;
const bool DBG_TIMING = true;
const bool DBG_BEEP = false;
const bool DBG_DRUM = false;
const bool DBG_CRASH = false;
const bool DBG_MINIFM = false;
const bool DBG_REVERB = false;
const bool DBG_EUCLIDEAN = false;
const bool DBG_JIGGLE = false;
const bool DBG_PATTERN = false;
const bool DBG_COPY = false;
const bool DBG_STARTUP = true;
const bool DBG_POT = false;

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

inline uint mult(uint a, uint b) {
  return (a * b) >> INTERNAL_BITS;
}

inline uint imult(int a, int b) {
  return (a * b) >> INTERNAL_BITS;
}

uint non_linear(uint val, uint n) {
  if (n == 0) return 1;
  else if (n == 1) return val;
  uint val2 = mult(val, val);
  if (n == 2) return (val + val2) >> 1;
  uint val3 = mult(val, val2);
  if (n == 3) return (val + val2 + 2 * val3) >> 2;
  uint val4 = mult(val, val3);
  return (val + (val2 << 1) + (val3 << 2) + val4 + (val4 << 3)) >> 4;
}

// efficient random bits from an LFSR (random quality is not important here!)
class LFSR16 {
private:
  uint16_t state;
public:
  LFSR16(uint16_t seed = 0xACE1) : state(seed) {}
  uint next() {
    uint16_t lsb = state & 0x1;
    state >>= 1;
    if (lsb) state ^= 0xB400;
    return lsb;
  }
  int scaled_bit(uint scale) {
    return scale * (next() ? 1 : -1);
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

static LFSR16 LFSR;

// wrapper for LEDs
// assumes values are INTERNAL_BITS and converts to LED_BITS
class LEDs {
private:
  uint n_leds = 4;
  // PINS - edit these values for the led pins
  const std::array<uint, 4> pins = {23, 32, 5, 2};
  const std::array<uint, 4> primes = {2, 3, 5, 7};
public:
  void init() {
    for (uint i = 0; i < n_leds; i++) {
      pinMode(pins[i], OUTPUT);
      bool ok = ledcAttach(pins[i], LED_FREQ, LED_BITS);
      if (DBG_STARTUP) Serial.printf("led %d %d/1\n", i, ok);
    }
    all_off();
  }
  void set(uint led, uint level) {
    analogWrite(pins[led], (level >> (INTERNAL_BITS - LED_BITS)) & LED_MAX);
  }
  void set(uint led, uint level, bool full) {
    set(led, level >> (full ? 0 : LED_DIM_BITS));
  }
  void on(uint led) {
    set(led, INTERNAL_MAX);
  }
  void on(uint led, bool on) {
    set(led, on ? INTERNAL_MAX : 0);
  }
  void off(uint led) {
    set(led, 0);
  }
  void all_on() {
    for (uint i = 0; i < n_leds; i++) on(i);
  }
  void all_off() {
    for (uint i = 0; i < n_leds; i++) off(i);
  }
  void start_up() {
    for (uint i = 0; i < 3; i++) {
      all_on();
      delay(100);
      all_off();
      delay(100);
    };
    if (DBG_STARTUP) Serial.printf("leds flashed\n");
  }
  void centre(bool full) {
    for (uint i = 0; i < n_leds; i++) set(i, ((i == 1 || i == 2) ? INTERNAL_MAX : 0) >> (full ? 0 : LED_DIM_BITS));
  }
  void bar(uint value, bool full) {
    uint quarter = INTERNAL_MAX >> 2;
    for (uint i = 0; i < n_leds; i++) {
      set(i, (value > quarter ? INTERNAL_MAX : value << 2) >> (full ? 0 : LED_DIM_BITS));
      value = value > quarter ? value - quarter : 0;
    }
  }
  void bar_right(uint value, bool full) {
    uint quarter = INTERNAL_MAX >> 2;
    for (uint i = 0; i < n_leds; i++) {
      set(n_leds - i + 1, (value > quarter ? INTERNAL_MAX : value << 2) >> (full ? 0 : LED_DIM_BITS));
      value -= quarter;
    }
  }
  void prime(uint value) {
     for (uint i = 0; i < n_leds; i++) on(!(value % primes[i]));
  }
  void gray(uint value) {
    uint g = ::gray(value);
    for (uint i = 0; i < n_leds; i++) on(i, g & (1 << i));
  }
  void index(uint value, bool full) {
    if (value) set(value - 1, LED_MAX, full);
    else all_off();
  }
  void bin(uint value, bool full) {
    for (uint i = 0; i < n_leds; i++) set(i, (value & (1 << i)) ? LED_MAX : 0, full);
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
    if (DBG_POT) Serial.printf("button %d, on %d, button_mask %d, n_buttons %d\n", idx, on, button_mask, n_buttons);
  }
  void voice(uint idx, bool on) {
    if (!button_mask) leds.on(idx, on);
  }
  void led_centre(bool full) {
    leds.centre(full);
  }
  void led_bar(uint value, bool full) {
    leds.bar(value, full);
  }
  void led_bar_right(uint value, bool full) {
    leds.bar_right(value, full);
  }
  void led_fm(uint value, bool full) {
    if (value < 0x7fff) led_bar(value << 1, full);
    else led_bar((value << 2) & 0xffff, full);
  }
  void led_prime(uint value) {
    leds.prime(value);
  }
  void led_gray(uint value) {
    leds.gray(value);
  }
  void led_5(uint value, bool full) {
    leds.index(value, full);
  }
  void led_bin(uint value, bool full) {
    leds.bin(value, full);
  }
  void led(uint led, uint value, bool full) {
    leds.set(led, value, full);
  }
  void led_clear() {
    leds.all_off();
  }
};

static CentralState STATE;

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
  int operator()(uint amp, uint phase) {
    // NOTE - this all works on INTERNAL_BITS, so lookup needs to reduce to TABLE_BITS if necessary
    uint half = TAU_N >> 1;
    uint quarter = half >> 1;
    int sign = 1;
    if (phase >= half) {
      phase = TAU_N - phase;
      sign = -1;
    };
    if (phase >= quarter) phase = half - phase;
    int result = sign * static_cast<int>(mult(amp, lookup(phase)));
    // int result = sign * static_cast<int>(((amp >> 1) * lookup(phase)) >> (INTERNAL_BITS - 1));  // avoid 32 bit overflow (TODO - needed?)
    return result;
  }
};

// implement sine as lookup
class Sine : public Quarter {
private:
  std::array<uint16_t, 1 + TABLE_N / 4> table;
public:
  Sine() : Quarter() {
    for (uint i = 0; i < 1 + (TABLE_N / 4); i++) table[i] = static_cast<uint16_t>(INTERNAL_MAX * sin(2 * PI * i / TABLE_N));
  }
  uint lookup(uint phase) override {
    // phase already in first quadrant
    return table[phase >> (TAU_BITS - TABLE_BITS)];
  }
};

Sine SINE;

// implement square (no need for table, can just use constant)
class Square : public Quarter {
public:
  Square() : Quarter() {}
  uint lookup(uint phase) override {
    return INTERNAL_MAX;
  }
};

Square SQUARE;

class Triangle : public Quarter {
public:
  Triangle() : Quarter() {}
  uint lookup(uint phase) override {
    // this should only be called in first quadrant and phase is internal bits
    return phase << 2;
  }
};

Triangle TRIANGLE;

int norm_phase(int phase) {
  return phase & TAU_MAX;
  // while (phase < 0) phase += TAU_MAX;
  // while (phase > TAU_MAX) phase -= TAU_MAX;
  // return phase;
}

class HiPass {
private:
  int prev_in = 0;
  int prev_out = 0;
  int alpha;
  int beta;
public:
  HiPass(int alpha) : alpha(alpha), beta(static_cast<int>(INTERNAL_MAX) - alpha) {};
  int next(int in, int alpha2) {
    alpha = alpha2;
    return next(in);
  }
  int next(int in) {
    prev_out = imult(alpha, in) - imult(alpha, prev_in) + imult(beta, prev_out);  // TODO - really?
    prev_in = in;
    return prev_out;
  }
};

class LoPass {
private:
  int prev_in = 0;
  int prev_prev_in = 0;
  int alpha;
  int beta;
public:
  LoPass(int alpha) : alpha(alpha), beta((INTERNAL_MAX - alpha) >> 1) {};
  int next(int in) {
    int out = imult(beta, in) + imult(alpha, prev_in) + imult(beta, prev_prev_in);
    prev_prev_in = prev_in;
    prev_in = in;
    return out;
  }
};

// class Latch {
// private:
//   bool saved = false;
//   int prev = 0;
// public:
//   int next(int value) {
//     if (saved) value = prev;
//     else prev = value;
//     saved = !saved;
//     return value;
//   }
// };

// generate the sound for each voice (which means tracking time and phase)
class Voice {
private:
  static const uint RESOLN = 1 << 12;  // hack because d > 16 bits so we get overflow
  uint time = 0;  // ticks since triggered
  int phase = 0;
  int fm_phase = 0;
  int fm_phase2 = 0;
  int noise[N_NOISE] = { 0 };
  uint noise_idx = 0;
  bool on = false;
  uint instantaneous_amp(uint amp, uint durn, uint dec, uint hard_deg) {
    uint low_amp = (amp & 0x7fff) << 1;
    uint hard_amp = mult(non_linear(low_amp, 4), non_linear(dec, hard_deg));
    uint phase = (INTERNAL_N * time) / (1 + 2 * durn);
    uint soft_amp = abs(SINE(mult(non_linear(low_amp, 4), dec), phase));
    return amp & 0x8000 ? soft_amp : hard_amp;
  }
public:
  uint idx;
  // parameters madified by UI (all INTERNAL_BITS)
  volatile uint amp;
  volatile uint freq;
  volatile uint durn;
  volatile uint fm;
  volatile int shift;
  Voice(uint idx, uint amp, uint freq, uint durn, uint fm, int shift) : idx(idx), amp(amp), freq(freq), durn(durn), fm(fm), shift(shift) {};
  void trigger() {
    on = true;
    time = 0;
    phase = 0;
    fm_phase = 0;
    fm_phase2 = 0;
  }
  int output() {
    if (!on) return 0;
    // sampling freq is 15.5 bits, internal is 16 bits, would like max durn to be about 4s.
    uint durn_scaled = (durn * durn) >> (INTERNAL_BITS - 2 - OVERSAMPLE_BITS);
    if (time >= durn_scaled) {
      STATE.voice(idx, false);    
      on = false;
      return 0;
    }
    uint nv_fm = fm;
    uint waveform = nv_fm >> 14;
    // 16 bits, but waveform removed
    uint fm_drum = (nv_fm & 0x7fff) << 1;
    uint fm_other = (nv_fm & 0x3fff) << 2;
    uint dec = (INTERNAL_MAX / RESOLN) * ((RESOLN * (durn_scaled - time)) / (durn_scaled + 1));
    int out = 0;
    switch (waveform) {
      case 0:
      case 1:
        out = drum(fm_drum, instantaneous_amp(amp, durn_scaled, dec, 2), 1 + non_linear(freq, 2), non_linear(dec, 2), non_linear(dec, 2));
        break;
      case 2:
        out = crash(fm_other, instantaneous_amp(amp, durn_scaled, dec, 4), 1 + non_linear(freq, 2), non_linear(dec, 3), non_linear(dec, 3));
        break;
      case 3:
      default:
        out = bass(fm_other, instantaneous_amp(amp, durn_scaled, dec, 1), 1 + non_linear(freq, 4));
        break;
    }
    time++;
    return out;
  }
  int bass(uint fm, uint amp, uint freq) {
    static LoPass lp(1 << 15);
    if (DBG_MINIFM && !random(DBG_LOTTERY)) Serial.printf("%d fm %d amp %d freq %d\n", idx, fm, amp, freq);
    uint low_freq = freq >> 4;
    fm_phase = norm_phase(fm_phase + (freq >> 3) + LFSR.n_bits(std::bit_width(fm >> 4)));
    phase = norm_phase(phase + low_freq + SINE(fm >> 7, fm_phase) + TRIANGLE(fm >> 9, fm_phase));
    int out = SINE(amp, phase);
    // return lp.next(out);
    return out;
  }
  int crash(uint fm, uint amp, uint freq, uint filter_dec, uint noise_dec) {
    if (DBG_CRASH && !random(DBG_LOTTERY)) Serial.printf("fm %d amp %d freq %d filter_dec %d noise_dec %d\n", fm, amp, freq, filter_dec, noise_dec);
    static HiPass hp(3 << 14);
    fm_phase = norm_phase(fm_phase + (fm << 1));
    fm_phase2 = norm_phase(fm_phase2 + fm + SQUARE(filter_dec >> 8, fm_phase));
    phase = norm_phase(phase + freq + TRIANGLE(INTERNAL_MAX >> 1, fm_phase2));
    int out = SINE(amp, phase);
    out += LFSR.scaled_bit(noise_dec >> 5);  // aliasing from noise at sample freq seems to help?
    out = hp.next(out, filter_dec << 1);
    out = imult(out, amp);
    return out;
  }
  int drum(uint fm, uint amp, uint freq, uint fm_dec, uint noise_dec) {
    if (DBG_DRUM && !random(DBG_LOTTERY)) Serial.printf("%d drum\n", idx);
    // separate fm, a 16 bit value whose 11 upper bits are varying, into 5 top and 6 low
    uint high_fm = (fm & 0xf800) >> 11;
    uint low_fm = (fm & 0x07e0) >> 5;
    freq >>= 6;
    phase = norm_phase(phase + freq + ((fm_dec * low_fm) >> 14));
    int out = SINE(amp, phase);
    out += LFSR.scaled_bit(mult(mult(amp, noise_dec), high_fm << 8));
    // out += (fmhi > 16 && !(time & 0xf)) ? LFSR.scaled_bit((final_amp * quad_dec * fmhi) >> 19) : 0;  // TODO wtf
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

std::array<Voice, 4> VOICES = {Voice(0, 0xffff, 160 << 4, 1200 << 4,   0, 0),
                               Voice(1, 0x7fff, 240 << 4, 1000 << 4, 240 << 4, 0),
                               Voice(2, 0x7fff, 213 << 4, 1300 << 4, 150 << 4, 0),
                               Voice(3, 0x7fff, 320 << 4,  900 << 4, 240 << 4, 0)};

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
    if (gray(ENABLED) & (1 << n)) {
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
      error.push_back(static_cast<int>(INTERNAL_MAX * (x - round(x))));
    }
    uint regular[n_beats] = {0};
    int penalty = INTERNAL_MAX;
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
  Euclidean() : Euclidean(2, 1, 0.5, INTERNAL_MAX, 0) {};  // used only as temp value in arrays
  bool operator==(const Euclidean other) {
    return other.n_places == n_places && other.n_beats == n_beats && other.n_main == n_main && other.prob == prob && other.voice == voice;
  }
  void jiggle() {
    // error is signed to MAX11 (MAX12 * 0.5) while prob is unisgned MAX12
    for (uint i = 0; i < n_beats - n_main; i++) {
      uint frac_m12 = (prob * static_cast<uint>(abs(error[i]))) >> 11;
      if (random(INTERNAL_MAX) < frac_m12) {
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
  EMA(uint bits, uint num, uint xtra, T state) : bits(bits), denom(1 << bits), xtra(xtra), state(state << xtra), num(num){};
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
  EMA<uint> ema = EMA<uint>(4, 3, 3, 0);
public:
  uint state;
  Pot(uint pin)
    : pin(pin){};
  void init() {
    pinMode(pin, INPUT);
  }
  void read_state() {
    state = (POT_MAX - ema.next(analogRead(pin)) - 1) << (INTERNAL_BITS - POT_BITS);
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
  static const uint thresh = 256;
  static const uint unknown = INTERNAL_N << 8;  // big enough to be unique
  std::array<bool, 4> enabled = {false, false, false, false};
  std::array<uint, 4> posn = {unknown, unknown, unknown, unknown};
  int active = -1;
  bool check_enabled(uint target, uint pot) {
    uint posn_error = abs(static_cast<int>(posn[pot]) - static_cast<int>(POTS[pot].state));
    if (DBG_POT && !random(DBG_LOTTERY >> 4)) Serial.printf("pot %d posn %d state %d posn error %d enabled %d active %d\n", pot, posn[pot], POTS[pot].state, posn_error, enabled[pot], active == pot);
    if (posn[pot] == unknown) posn[pot] = POTS[pot].state;
    else if (posn_error > thresh) {
      // exclude pots that have not been moved
      posn[pot] = POTS[pot].state;
      if (active != pot) {
        STATE.led_clear();
        active = pot;
      }
    }
    uint target_error = abs(static_cast<int>(target) - static_cast<int>(POTS[pot].state));
    if (!enabled[pot] && target_error < thresh) enabled[pot] = true;
    if (DBG_POT && enabled[pot] && !random(DBG_LOTTERY >> 6)) Serial.printf("enabled pot %d target %d state %d error %d\n", pot, target, POTS[pot].state, target_error);
    return enabled[pot];
  }
protected:
  void disable() {
    std::fill(std::begin(enabled), std::end(enabled), false);
    std::fill(std::begin(posn), std::end(posn), unknown);
    active = -1;
  }
  void update(volatile uint* destn, uint zero, int bits, uint pot) {
    int target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (check_enabled(target, pot)) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
    if (pot == active) {
      if (DBG_POT && !random(DBG_LOTTERY >> 6)) Serial.printf("led_bar %d %d %d\n", pot, target, enabled[pot]);
      STATE.led_bar(target, enabled[pot]);
    }
  }
  void update(volatile uint* destn, uint pot) {
    update(destn, 0, 0, pot);
  }
  void update_no_msb(volatile uint* destn, uint pot) {
    if (check_enabled(*destn, pot)) *destn = POTS[pot].state;
    if (pot == active) STATE.led_bar((*destn & 0x7fffu) << 1, enabled[pot]);
  }
  void update_fm(volatile uint* destn, uint pot) {
    if (check_enabled(*destn, pot)) *destn = POTS[pot].state;
    if (pot == active) STATE.led_fm(*destn, enabled[pot]);
  }
  void update_subdiv(volatile uint* destn, uint zero, int bits, uint pot) {
    int pot_target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (check_enabled(pot_target, pot)) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
    if (pot == active) STATE.led_prime(SUBDIVS[*destn]);  // TODO - no enabled?
  }
  void update_prime(volatile uint* destn, uint zero, int bits, uint pot) {
    int pot_target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (check_enabled(pot_target, pot)) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
    if (pot == active) STATE.led_prime(*destn);  // TODO - no enabled?
  }
  void update_prime(float* destn, uint scale, uint pot) {
    int pot_target = *destn * INTERNAL_MAX;
    if (check_enabled(pot_target, pot)) *destn = POTS[pot].state / INTERNAL_MAX;
    if (pot == active) STATE.led_prime(*destn * scale);  // TODO - no enabled?
  }
  void update_lr(float* destn, uint pot, bool p1) {
    static const int EDGE = 4;  // guarantee 0-1 float full range
    int target = *destn * INTERNAL_MAX;
    if (check_enabled(target, pot))
      *destn = max(0.0f, min(1.0f, static_cast<float>((static_cast<int>(POTS[pot].state) - EDGE)) / (static_cast<int>(INTERNAL_MAX) - 2 * EDGE)));
    if (pot == active) {
      STATE.led(p1 ? 0 : 2, *destn * INTERNAL_MAX, enabled[pot]);
      STATE.led(p1 ? 1 : 3, (1 - *destn) * INTERNAL_MAX, enabled[pot]);
    }
  }
  void update_gray(volatile uint* destn, uint zero, int bits, uint pot) {
    uint mask = (1 << bits) - 1;
    uint target = ((static_cast<int>(*destn) - zero) & mask) << (INTERNAL_BITS - bits);
    if (check_enabled(target, pot)) *destn = (zero + (POTS[pot].state >> (INTERNAL_BITS - bits))) & mask;
    if (pot == active) STATE.led_gray(*destn);  // TODO - no enabled?
  }
  void update_signed(volatile int* destn, uint pot) {
    uint target = (*destn + (INTERNAL_MAX >> 1));
    if (check_enabled(target, pot)) *destn = POTS[pot].state - (INTERNAL_MAX >> 1);
    if (pot == active) {
      if (abs(*destn) < 16) STATE.led_centre(enabled[pot]);
      else if (*destn >= 0) STATE.led_bar(((INTERNAL_MAX >> 1) - *destn) << 1, enabled[pot]);
      else STATE.led_bar_right(((INTERNAL_MAX >> 1) + *destn) << 1, enabled[pot]);
    }
  }
  void update_5(uint* destn, uint pot) {
    uint target = (INTERNAL_MAX * *destn) / 5;
    if (check_enabled(target, pot)) *destn = (POTS[pot].state * 5) / (INTERNAL_MAX + 1);
    if (pot == active) STATE.led_5(*destn, enabled[pot]);
  }
  void update_bin(uint* destn, uint pot) {
    uint shift = INTERNAL_BITS - 4;  // TODO ?
    uint target = *destn << shift;
    if (check_enabled(target, pot)) *destn = POTS[pot].state >> shift;
    if (pot == active) STATE.led_bin(*destn, enabled[pot]);
  }
  void update_frac(volatile uint* destn, uint lo, uint hi, uint pot) {
    uint target = INTERNAL_MAX * (static_cast<float>(*destn - lo) / (hi - lo));
    if (check_enabled(target, pot)) *destn = lo + (hi - lo) * (static_cast<float>(POTS[pot].state) / INTERNAL_MAX);
    if (pot == active) STATE.led_bar(target, enabled[pot]);
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
      update_no_msb(&voice.amp, 0);
      update(&voice.freq, 1);
      update(&voice.durn, 2);
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
      update(&BPM, 30, -6, 0);
      update_subdiv(&SUBDIV_IDX, 0, -12, 1);
    } else {
      disable();
    }
  }
};

static GlobalButtons GLOBAL_BUTTONS;

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

static EuclideanVault VAULTS[2] = {EuclideanVault(16, 0.666, 0.5, 0, 0), EuclideanVault(25, 0.666, 0.5, 0x7fff, 2)};

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
      update(&vault.prob, 3);
      vault.apply_edit(false);
    } else if (editing) {
      vault.apply_edit(true);
      disable();
      editing = false;
    }
  }
};

static EuclideanButtons EUCLIDEAN_BUTTONS_LEFT = EuclideanButtons(0x3, VAULTS[0]);
static EuclideanButtons EUCLIDEAN_BUTTONS_RIGHT = EuclideanButtons(0xc, VAULTS[1]);

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
  Reverb() : head(TapeEMA(this, INTERNAL_BITS, INTERNAL_MAX, 4)) {};  // by default disabled
  int next(int val) {
    write = (write + 1) % max(1u, size);  // can't be "& mask" because size can vary
    return head.next(val);
  }
};

static Reverb REVERB = Reverb<13>();

// post-process - outer two buttons
class PostButtons : public PotsReader {
private:
  uint buffer_frac = ((LOCAL_BUFFER_SIZE * (INTERNAL_MAX >> 1)) / MAX_LOCAL_BUFFER_SIZE) + (LOCAL_BUFFER_SIZE & 0x1 ? 0 : (INTERNAL_MAX >> 1));
public:
  PostButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0x9) {
      update(&REVERB.size, 0, -3, 0);
      update(&REVERB.head.num, 1, 0, 1);
      update_frac(&COMP_BITS, 0, MAX_COMP_BITS, 2);
      update(&SHIFT_BITS, 0, -12, 3);
    } else {
      disable();
    }
  }
};

static PostButtons POST_BUTTONS;

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

static PerfButtons PERFORMANCE_BUTTONS;

// post-process - outer two buttons
class SysButtons : public PotsReader {
private:
  uint buffer_frac = ((LOCAL_BUFFER_SIZE * (INTERNAL_MAX >> 1)) / MAX_LOCAL_BUFFER_SIZE) + (LOCAL_BUFFER_SIZE & 0x1 ? 0 : (INTERNAL_MAX >> 1));
public:
  SysButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0xf) {
      float prev = buffer_frac;
      update_no_msb(&buffer_frac, 0);
      if (buffer_frac != prev) {
        LOCAL_BUFFER_SIZE = max(MIN_LOCAL_BUFFER_SIZE, min(MAX_LOCAL_BUFFER_SIZE, (buffer_frac & (INTERNAL_MAX >> 1)) * MAX_LOCAL_BUFFER_SIZE / (INTERNAL_MAX >> 1)));
        if (buffer_frac > (INTERNAL_MAX >> 1)) {
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

static SysButtons SYS_BUTTONS;

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

int compress(int out, uint bits, uint ceiling) {
  int sign = sgn(out);
  uint absolute = abs(out);
  bits = min(bits, ceiling);
  for (uint i = (ceiling - bits); i < ceiling; i++) {
    uint limit = 1 << i;
    if (absolute > limit) absolute = limit + ((absolute - limit) >> 1);
    else break;
  }
  return sign * static_cast<int>(absolute);
}

// apply post-processing
uint post_process(int amp) {
  // apply compressor
  int soft_clipped = compress(amp, COMP_BITS, MAX_COMP_BITS);
  // apply quantisation
  int shifted = (soft_clipped >> SHIFT_BITS) << SHIFT_BITS;  // implementation dependent but signed here
  // apply reverb
  // int reverbed = soft_clipped;
  int reverbed = REVERB.next(shifted);
  // if (DBG_REVERB && !random(DBG_LOTTERY)) Serial.printf("reverb %d -> %d\n", soft_clipped, reverbed);
  // hard clip
  int offset = reverbed + 0x80;
  int hard_clipped = max(0, min(0xff, offset));
  // if (DBG_VOLUME && !random(DBG_LOTTERY)) Serial.printf("comp %d; vol %d; soft %d; hard %d\n", 8 - COMP_BITS, vol, soft_clipped, hard_clipped);
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
      for (Voice& voice : VOICES) vol += voice.output();
      data[i] = post_process(vol >> 4);
    }
  }
};

// run the ui on other core
static TaskHandle_t ui_handle = NULL;

// main loop on other core for slow/ui operations
void ui_loop(void*) {
  uint count = 0;
  while (1) {
    for (Pot& p : POTS) p.read_state();
    for (Button& b : VOICE_BUTTONS) b.read_state();  // each button individually
    GLOBAL_BUTTONS.read_state();                     // central pair
    EUCLIDEAN_BUTTONS_LEFT.read_state();             // left pair
    EUCLIDEAN_BUTTONS_RIGHT.read_state();            // right pair
    POST_BUTTONS.read_state();                       // outer pair
    PERFORMANCE_BUTTONS.read_state();                // left triplet
    EDIT_BUTTONS.read_state();                       // right triplet
    SYS_BUTTONS.read_state();                        // all
    if (count < 2 && DBG_STARTUP) Serial.printf("ui loop %d/2\n", ++count);
    vTaskDelay(1);
  }
}

static Audio AUDIO(Trigger(VAULTS[0], VOICES[0], true, false), 
                   Trigger(VAULTS[0], VOICES[1], false, false), 
                   Trigger(VAULTS[1], VOICES[2], true, true),
                   Trigger(VAULTS[1], VOICES[3], false, true));
static uint8_t BUFFER[2][MAX_LOCAL_BUFFER_SIZE];
volatile static uint dma_idx = 0;  // not needed, afaict, because dma buffer is always large enough for what we have
static SemaphoreHandle_t dma_semaphore;
static BaseType_t dma_flag = pdFALSE;
static dac_continuous_handle_t dac_handle;
volatile static uint dma_time = 0;
volatile static uint dma_errors = 0;

// fast copy of existing data into buffer - seems to take a very consistent 200us
static IRAM_ATTR bool dac_callback(dac_continuous_handle_t handle, const dac_event_data_t* event, void* user_data) {
  unsigned long start = micros();
  uint loaded = 0;
  dac_continuous_write_asynchronously(handle, static_cast<uint8_t*>(event->buf), event->buf_size, BUFFER[dma_idx], LOCAL_BUFFER_SIZE, &loaded);
  if (loaded != LOCAL_BUFFER_SIZE) dma_errors += 1;
  if (event->buf_size != DMA_BUFFER_SIZE) dma_errors += 1;
  if (event->write_bytes != 2 * LOCAL_BUFFER_SIZE) dma_errors += 1;
  dma_idx = !dma_idx;
  xSemaphoreGiveFromISR(dma_semaphore, &dma_flag);  // flag refill
  dma_time = micros() - start;
  return false;
}

// slow fill of buffer - time depends on number of voices active (obvs)
void update_buffer() {
  static uint burst = 0;
  static uint late = 0;
  static EMA<uint> avg_duty = EMA<uint>(8, 1, 4, 100);
  unsigned long start = micros();
  AUDIO.generate(LOCAL_BUFFER_SIZE, BUFFER[dma_idx]);
  uint calc_time = micros() - start;
  uint duty = (100 * (dma_time + calc_time)) / REFRESH_US;
  if (duty > 100) late++;
  uint avg = avg_duty.next(duty);
  if (DBG_TIMING && !burst && !random(DBG_LOTTERY / max(1u, LOCAL_BUFFER_SIZE >> 3))) {
    burst = 3;
    Serial.println("---");
  }
  if (burst) {
    Serial.printf("buffer %d; dma %d, calc %d, duty %d/%d; errors %d, late %d\n", LOCAL_BUFFER_SIZE, dma_time, calc_time, duty, avg, dma_errors, late);
    burst--;
  }
  static uint startup_count = 0;
  if (startup_count < 2 && DBG_STARTUP) Serial.printf("update buffer %d/2\n", ++startup_count);
}

// refill buffer when flagged
void loop() {
  for (;;) {
    if (xSemaphoreTake(dma_semaphore, portMAX_DELAY) == pdTRUE) update_buffer();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("xqria dice hola al mundo");
  if (DBG_STARTUP) Serial.printf("buttons %d/4\n", VOICE_BUTTONS.size());
  for (Button& b : VOICE_BUTTONS) b.init();
  if (DBG_STARTUP) Serial.printf("pots %d/4\n", POTS.size());
  for (Pot& p : POTS) p.init();
  STATE.init();

  int ok = xTaskCreatePinnedToCore(&ui_loop, "UI Loop", 10000, NULL, 1, &ui_handle, 0);
  if (DBG_STARTUP) Serial.printf("task created %d/1\n", ok);

  update_buffer();  // first callback happens immediately
  if (DBG_STARTUP) Serial.printf("first buffer updated\n");
  dma_semaphore = xSemaphoreCreateBinary();
  if (DBG_STARTUP) Serial.printf("semaphore created %d/1\n", dma_semaphore != nullptr);
  dac_continuous_config_t dac_config = {
    .chan_mask = DAC_CHANNEL_MASK_CH0,
    .desc_num = 8,  // 2 allows pinpong for large buffers, but a larger value seems to help small buffers
    .buf_size = DMA_BUFFER_SIZE,
    .freq_hz = SAMPLE_RATE_HZ << OVERSAMPLE_BITS,
    .offset = 0,
    .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
    .chan_mode = DAC_CHANNEL_MODE_SIMUL  // not used for single channel
  };
  ok = dac_continuous_new_channels(&dac_config, &dac_handle);
  if (DBG_STARTUP) Serial.printf("dac new channels %d/0\n", ok);
  ok = dac_continuous_enable(dac_handle);
  if (DBG_STARTUP) Serial.printf("dac continuous %d/0\n", ok);
  dac_event_callbacks_t callbacks = {
    .on_convert_done = dac_callback,
    .on_stop = nullptr
  };
  ok = dac_continuous_register_event_callback(dac_handle, &callbacks, nullptr);
  if (DBG_STARTUP) Serial.printf("callback registered %d/0\n", ok);
  ok = dac_continuous_start_async_writing(dac_handle);
  if (DBG_STARTUP) Serial.printf("asynch started %d/0\n", ok);
};
