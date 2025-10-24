
#include <Arduino.h>
#include <atomic>
#include <array>
#include <numeric>
#include <mutex>
#include <thread>
#include "esp_timer.h"
#include "driver/dac.h"
#include "math.h"

const uint TIMER_PERIOD_US = 50;  // 40khz is 25 but we don't seem to get that high - see DBG_TIMING
const uint LOWEST_F_HZ = 20;  // assuming integer phase increment
const uint NSAMPLES = 1000000 / (LOWEST_F_HZ * TIMER_PERIOD_US);  // framing things this way gives constant frequency even if period changes
const uint BEAT_SCALE = 1000000 * 60 / (4 * TIMER_PERIOD_US);  // convert to bpm assuming 4/4 (not sure this is correct)
const uint PHASE_EXTN = 2;  // extra bits for phase to extend low freqs
const uint NSAMPLES_EXTN = NSAMPLES << PHASE_EXTN; 
const bool FM_NOISE = true;
const uint NOISE_BITS = 6;
const uint N_NOISE = 1 << NOISE_BITS;
const uint NOISE_MASK = N_NOISE - 1;
const std::array<uint, 16> SUBDIVS = {5, 10, 12, 15, 20, 24, 25, 30, 35, 36, 40, 45, 48, 50, 55, 60};  // by luck length is power of 2
const uint DBG_LOTTERY = 50000;

// global parameters that can be changed during use
volatile static uint BPM = 90;
volatile static uint SWING = 0;
volatile static uint SUBDIV = 15;
volatile static uint COMP_BITS = 0;

const uint N6 = 1 << 6;
const uint MAX6 = N6 - 1;
const uint N7 = 1 << 7;
const uint MAX7 = N7 - 1;
const uint N8 = 1 << 8;
const uint MAX8 = N8 - 1;
const uint N10 = 1 << 10;
const uint MAX10 = N10 - 1;
const uint N11 = 1 << 11;
const uint MAX11 = N11 - 1;
const uint N12 = 1 << 12;
const uint MAX12 = N12 - 1;

const bool DBG_PATTERN = false;
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

template <typename T> int sgn(T val) {return (T(0) < val) - (val < T(0));}

template <typename T> T series_exp(T val, uint n) {
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
  LFSR16(uint16_t seed = 0xACE1) : state(seed) {}
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
        if (line++ == 80) {line = 0; Serial.println();}
      }
      bits = bits << 1 | bit;
      n--;
    }
    return bits;
  }
};

LFSR16 LFSR = LFSR16();

// wrapper for LEDs
class LEDs {
private:
  uint n_leds = 4;
  std::array<uint, 4> pins = {23, 32, 5, 2};
public:
  void init() {
    for (uint i = 0; i < n_leds; i++) pinMode(pins[i], OUTPUT);
    all_off();
  }
  void set(uint led, uint level) {analogWrite(pins[led], level & 0xff);}
  void on(uint led) {digitalWrite(pins[led], HIGH);}
  void on(uint led, bool on) {digitalWrite(pins[led], on ? HIGH : LOW);}
  void off(uint led) {digitalWrite(pins[led], LOW);}
  void all_on() {for (uint i = 0; i < n_leds; i++) on(i);}
  void all_off() {for (uint i = 0; i < n_leds; i++) off(i);}
  void start_up() {
    all_on(); delay(100);
    all_off(); delay(100);
    all_on(); delay(100);
    all_off();
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
  }
  void voice(uint idx, bool on) {if (!button_mask) leds.on(idx, on);}
  void pot(uint idx) {leds.on(idx);}
  void pot_clear() {leds.all_off();}
};

CentralState STATE = CentralState();

// quarter wave lookup (don't need to store complete sine wave; only first quarter)
class Quarter {
private:
  virtual uint lookup(uint phase);
public:
  Quarter() = default;
  int operator()(uint amp, int phase) {
    int sign = 1;
    if (phase >= NSAMPLES / 2) {
      phase = NSAMPLES - phase;
      sign = -1;
    };
    if (phase >= NSAMPLES / 4) phase = (NSAMPLES / 2) - phase;
    return sign * ((amp * lookup(phase)) >> 12);
  }
};

// implement sine as lookup
class Sine : public Quarter {
private:
  std::array<uint, 1 + NSAMPLES / 4> table;
public:
  Sine() : Quarter() {for (uint i = 0; i < 1 + NSAMPLES / 4; i++) table[i] = MAX12 * sin(2 * PI * i / NSAMPLES);}
  uint lookup(uint phase) override {return table[phase];}
};

Sine SINE;

// implement square (no need for table, can just use constant)
class Square : public Quarter {
public:
  Square() : Quarter() {}
  uint lookup(uint phase) override {return MAX12;}
};

Square SQUARE;

// generate the sound for each voice (which means tracking time and phase)
class Voice {
private:
  uint idx;
  uint time = 0;
  int phase = 0;
  int fm_phase = 0;
  int noise[N_NOISE] = {0};
  uint noise_idx = 0;
public:
  // parameters madified by UI
  volatile uint amp;    // 12 bits
  volatile uint freq;   // 12 bits
  volatile uint durn;   // 12 bits
  volatile uint fm;     // 12 bits
  Voice(uint idx, uint amp, uint freq, uint durn, uint fm)
    : idx(idx), amp(amp), freq(freq), durn(durn), fm(fm){};
  void trigger() {
    time = 0;
    phase = 0;
  }
  int output_12() {
    uint durn_scaled = durn << 2;
    if (time == durn_scaled) STATE.voice(idx, false);
    if (time > durn_scaled) return 0;
    uint nv_fm = fm;
    uint fm_low10 = nv_fm & MAX10;
    uint fm_low11 = max(0u, MAX11 - (nv_fm & MAX11) - 1);
    uint nv_amp = amp;
    uint amp_11 = amp & MAX11;
    uint amp_scaled = series_exp(amp_11, 3) >> 15;
    uint linear_dec = MAX12 * (durn_scaled - time) / (durn_scaled + 1);
    uint power_dec = series_exp(linear_dec, 2) >> 11;
    uint hard = amp_scaled * power_dec >> 12;
    uint env_phase = (NSAMPLES * time) / (1 + 2 * durn_scaled);
    uint soft = abs(SINE((amp_scaled * linear_dec) >> 11, env_phase));
    uint final_amp = nv_amp & N11 ? soft : hard;
    uint nv_freq = freq;
    uint freq_scaled = 1 + (series_exp(nv_freq, 2) >> 14);
    if ((DBG_BEEP | DBG_CRASH | DBG_DRUM | DBG_MINIFM) && !idx && !random(DBG_LOTTERY))
      Serial.printf("amp_12 %d, amp_11 %d, amp_scaled %d, linear %d, quad %d, hard %d, soft %d, amp %d, freq %d, freq_scaled %d, fm %d\n", 
                    nv_amp & N11, amp_11, amp_scaled, linear_dec, power_dec, hard, soft, final_amp, nv_freq, freq_scaled, nv_fm);
    int out = 0;
    switch (nv_fm >> 10) {
      case 0:
      out = drum(fm_low10, final_amp, freq_scaled, power_dec);
      break;
      case 1:
      out = crash(fm_low10, final_amp, freq_scaled);
      break;
      case 2:
      case 3:
      default:
      out = minifm(fm_low11, final_amp, freq_scaled);
      break;
    }
    time++;
    return out >> 4;
  }
  int minifm(uint fm, uint final_amp, uint freq_scaled) {
    // hand tuned for squelchy noises
    if (DBG_TONE && !random(DBG_LOTTERY)) Serial.printf("%d fm %d\n", idx, fm);
    fm_phase += series_exp(fm, 3) >> 22;
    while (fm_phase >= NSAMPLES_EXTN) fm_phase -= NSAMPLES_EXTN;
    phase += freq_scaled + SINE(max(1u, freq_scaled), fm_phase >> PHASE_EXTN);  // sic
    while (phase < 0) phase += NSAMPLES_EXTN;
    while (phase >= NSAMPLES_EXTN) phase -= NSAMPLES_EXTN;
    int out = SINE(final_amp, phase >> PHASE_EXTN);
    if (DBG_MINIFM && !idx && !random(DBG_LOTTERY)) 
      Serial.printf("time %d, phase %d, out %d\n", time, phase, out);
    return out;
  }
  int crash(uint fm, uint final_amp, uint freq_scaled) {
    if (DBG_TONE && !random(DBG_LOTTERY)) Serial.printf("%d crash\n", idx);
    noise[noise_idx] = LFSR.next() ? 1 : -1;
    int out = 0;
    int count = 0;  // ugh must be signed
    uint conv_phase = 0;
    uint conv_freq = (1 + fm) << (PHASE_EXTN);
    uint i = 0;
    while (i < N_NOISE) {
      conv_phase += conv_freq;
      while (conv_phase >= NSAMPLES_EXTN) conv_phase -= NSAMPLES_EXTN;
      out += (noise[(noise_idx + i++) & NOISE_MASK] * SINE(N12, conv_phase >> PHASE_EXTN));
      count++;
    }
    noise_idx = (noise_idx + 1) & NOISE_MASK;
    out /= count;
    phase += freq_scaled;
    while (phase < 0) phase += NSAMPLES_EXTN;
    while (phase >= NSAMPLES_EXTN) phase -= NSAMPLES_EXTN;
    out += SQUARE(MAX8, phase >> PHASE_EXTN);
    out = (out * static_cast<int>(final_amp)) >> 8;
    if (DBG_CRASH && !random(DBG_LOTTERY))
      Serial.printf("count %d, conv_freq %d, out %d\n", count, conv_freq, out);
    return out;
  }
  int drum(uint fm, uint final_amp, uint freq_scaled, uint quad_dec) {
    if (DBG_TONE && !random(DBG_LOTTERY)) Serial.printf("%d drum\n", idx);
    freq_scaled = freq_scaled >> 2;
    phase += freq_scaled;
    phase += (quad_dec * (fm >> 8)) >> 8;
    while (phase < 0) phase += NSAMPLES_EXTN;
    while (phase >= NSAMPLES_EXTN) phase -= NSAMPLES_EXTN;
    int out = SINE(final_amp, phase >> PHASE_EXTN);
    if (DBG_DRUM && !idx && !random(DBG_LOTTERY)) 
      Serial.printf("time %d, fm %d, out %d\n", time, fm, out);
    return out;
  }
  int beep(uint fm, uint final_amp, uint freq_scaled) {
    if (DBG_TONE && !random(DBG_LOTTERY)) Serial.printf("%d beep\n", idx);
    phase += freq_scaled + LFSR.n_bits(fm >> 6);
    while (phase < 0) phase += NSAMPLES_EXTN;
    while (phase >= NSAMPLES_EXTN) phase -= NSAMPLES_EXTN;
    int out = SINE(final_amp, phase >> PHASE_EXTN);
    if (DBG_BEEP && !idx && !random(DBG_LOTTERY)) 
      Serial.printf("time %d, phase %d, out %d\n", time, phase, out);
    return out;
  }
};

// initial values no longer sounds good (TODO - improve)
std::array<Voice, 4> VOICES = {Voice(0, MAX10, 160, 1200, 0),
                               Voice(1, MAX10, 240, 1000, 240),
                               Voice(2, MAX10, 213, 1300, 150),
                               Voice(3, MAX10, 320,  900, 240)};

// standard euclidean pattern
// TODO - add variations biased towards beats with largest errors
class Euclidean {
private:
  uint n_places;
  uint n_beats;
  float frac_main;
  uint prob;
  uint voice;
  uint current_place = 0;
  std::vector<uint> place;
  std::vector<float> error;
  std::vector<uint> index_by_error;
  std::vector<bool> is_main;
  float n_main;
  std::vector<int> index_by_place;
  void trigger(uint delta) {
    VOICES[voice + delta].trigger();
    STATE.voice(voice + delta, true);
  }
public:
  // 1 <= n_beats <= n_places
  Euclidean(uint n_places, uint n_beats, float frac_main, uint prob, uint voice)
    : n_places(n_places), n_beats(n_beats), frac_main(max(0.0f, min(1.0f, frac_main))), prob(prob), voice(voice) {
    for (uint i = 0; i < n_beats; i++) {
      float x = n_places * i / static_cast<float>(n_beats);
      place.push_back(round(x));
      error.push_back(x - place[i]);
    };
    index_by_error.resize(n_beats, 0);
    std::iota(index_by_error.begin(), index_by_error.end(), 0);
    std::sort(index_by_error.begin(), index_by_error.end(), [this](int i, int j) {
      return abs(this->error[i]) < abs(this->error[j]);
    });
    n_main = max(0u, min(static_cast<uint>(round(n_beats * frac_main)), n_beats));
    is_main.resize(n_beats, false);
    for (uint i = 0; i < n_main; i++) is_main[index_by_error[i]] = true;
    index_by_place.resize(n_places, -1);
    for (uint i = 0; i < n_beats; i++) index_by_place[place[i]] = i;
  };
  Euclidean() : Euclidean(2, 1, 0.5, MAX12, 0){};  // used only as temp value in arrays
  bool operator==(const Euclidean other) {
    return other.n_places == n_places && other.n_beats == n_beats && other.n_main == n_main && other.prob == prob && other.voice == voice;
  }
  void on_beat(bool main) {
    int beat = index_by_place[current_place];
    if (beat != -1 && is_main[beat] == main && random(MAX12) < prob) {
      trigger(main ? 0 : 1);
      if (DBG_PATTERN) Serial.printf("%d: %d/%d %d (%d)\n", voice, current_place, n_places, beat, main);
    }
    if (main) {  // minor is done before main
      if (++current_place == n_places) current_place = 0;
    }
  }
};

// exponential moving average used for smoothing time series
template <typename T> class EMA {
private:
  uint bits;
  uint denom;
  uint xtra;
  T state;
protected:
  virtual T get_state() {return state;}
  virtual void set_state(T s) {state = s;}
public:
  uint num;
  EMA(uint bits, uint num, uint xtra, T state)
  : bits(bits), num(num), denom(1 << bits), xtra(xtra), state(state) {};
  T next(T val) {
    set_state((get_state() * static_cast<T>(denom - num) + (val << xtra) * static_cast<T>(num)) >> bits);
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
  Pot(uint pin) : pin(pin) {};
  void init() {pinMode(pin, INPUT);}
  void set_state() {state = MAX12 - ema.next(analogRead(pin)) - 1;}  // inverted and hack to zero
};

// all the pots
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
  Button(uint idx, uint pin)
    : idx(idx), pin(pin){};
  void init() {pinMode(pin, INPUT_PULLUP);}
  void set_state() {
    changed = false;
    bool current = !digitalRead(pin);  // inverted
    uint now = millis();
    if (tmp_change == 0 || current != tmp_pressed) {
      tmp_pressed = current;
      tmp_change = now;
    } else if (now - tmp_change > debounce && tmp_pressed != pressed) {
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
  std::array<bool, 4> enabled = { false, false, false, false };
  const std::array<uint, 4> prime = {2, 3, 5, 7};
protected:
  void disable() {
    std::fill(std::begin(enabled), std::end(enabled), false);
  }
  void update(volatile uint* destn, uint pot) {
    if (!enabled[pot] && abs(static_cast<int>(*destn) - static_cast<int>(POTS[pot].state)) < thresh) {
      enabled[pot] = true;
      STATE.pot(pot);
    }
    if (enabled[pot]) *destn = POTS[pot].state;
  }
  void update(float* destn, uint pot) {
    static const int EDGE = 4;
    if (!enabled[pot] && abs(static_cast<int>(*destn * MAX12) - static_cast<int>(POTS[pot].state)) < thresh) {
      enabled[pot] = true;
      STATE.pot(pot);
    }
    if (enabled[pot]) *destn = max(0.0f, min(1.0f, static_cast<float>((static_cast<int>(POTS[pot].state) - EDGE)) / (static_cast<int>(MAX12) - 2 * EDGE)));
  }
  // -ve bits imply value loaded into destn is smaller than pot
  void update(volatile uint* destn, uint zero, int bits, uint pot) {
    int target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    if (!enabled[pot] && abs(target - static_cast<int>(POTS[pot].state)) < thresh) {
      enabled[pot] = true;
      STATE.pot(pot);
    }
    if (enabled[pot]) *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
  }
  // -ve bits imply value loaded into destn is smaller than pot
  void update_prime(uint* destn, uint zero, uint bits, uint pot) {
    int target = bits < 0 ? (*destn - zero) << -bits : (*destn - zero) >> bits;
    enabled[pot] = enabled[pot] || abs(target - static_cast<int>(POTS[pot].state)) < thresh;
    if (enabled[pot]) {
      *destn = zero + (bits < 0 ? POTS[pot].state >> -bits : POTS[pot].state << bits);
      STATE.pot_clear();
      for (uint i = 0; i < 4; i++) if (*destn % prime[i] == 0) STATE.pot(i);
    }
  }
};

// subclass button to edit voice parameters
class VoiceButton : public Button, public PotsReader {
private:
bool editing = false;
  Voice& voice;
  void take_action() {
    if (pressed && STATE.n_buttons == 1) {
      editing = true;
      update(&voice.amp, 0);
      update(&voice.freq, 1);
      update(&voice.durn, 2);
      update(&voice.fm, 3);
      if (DBG_VOICE) Serial.printf("a %d, f %d, d %d, n %d\n", voice.amp, voice.freq, voice.durn, voice.fm);
    } else {
      if (editing) {
        Serial.printf("Voice(%d, %d, %d, %d, %d)\n", idx, voice.amp, voice.freq, voice.durn, voice.fm);
        editing = false;
      }
      disable();
    }
  }
public:
  VoiceButton(uint idx, uint pin, Voice& voice) : Button(idx, pin), PotsReader(), voice(voice){};
};

std::array<VoiceButton, 4> BUTTONS = { VoiceButton(0, 18, VOICES[0]),
                                       VoiceButton(1, 4, VOICES[1]),
                                       VoiceButton(2, 15, VOICES[2]),
                                       VoiceButton(3, 19, VOICES[3]) };

// global parameters - hold down middle two buttons
class GlobalButtons : public PotsReader {
public:
  GlobalButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0x6) {
      update(&BPM, 30, -4, 0);
      update(&SUBDIV, 0, -8, 1);
    } else {
      disable();
    }
  }
};

GlobalButtons GBUTTONS = GlobalButtons();

// handle passing of complex state between threads (everything else is a volatile uint, i think, but the pattern is complex)
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
  uint voice;
  std::mutex access;
  Euclidean euclideans[2];
  bool updated = false;
  uint current = 0;
  uint next = 1;
public:
  uint n_places;
  float frac_beats;
  float frac_main;
  uint prob;
  EuclideanVault(uint n_places, float frac_beats, float frac_main, uint prob, uint voice)
    : n_places(n_places), frac_beats(frac_beats), frac_main(frac_main), prob(prob), voice(voice) {
    euclideans[0] = Euclidean(n_places, max(1u, static_cast<uint>(n_places * frac_beats)), frac_main, prob, voice);
    euclideans[1] = Euclidean(n_places, max(1u, static_cast<uint>(n_places * frac_beats)), frac_main, prob, voice);
  }
  void apply_edit(bool dump) {
    std::lock_guard<std::mutex> lock(access);
    uint n_beats = max(1u, static_cast<uint>(n_places * frac_beats));
    Euclidean candidate = Euclidean(n_places, n_beats, frac_main, prob, voice);
    if (euclideans[updated ? next : current] != candidate) {
      if (DBG_EUCLIDEAN) Serial.printf("Euclidean(%d, %d, %f.3, %d, %d)\n", n_places, n_beats, frac_main, prob, voice);
      euclideans[next] = candidate;
      updated = true;
    }
  };
  Euclidean* get() {
    std::unique_lock<std::mutex> lock(access, std::defer_lock);
    if (lock.try_lock() && updated) {std::swap(current, next); updated = false;}
    return &euclideans[current];
  }
};

EuclideanVault vault1 = EuclideanVault(16, 0.5, 0.5, MAX12, 0);
EuclideanVault vault2 = EuclideanVault(25, 0.5, 0.5, MAX12, 2);

// edit patterns - hold down left or right two buttons (mask)
class EuclideanButtons : public PotsReader {
private:
  uint mask;
  EuclideanVault &vault;
  bool editing = false;
public:
  EuclideanButtons(uint mask, EuclideanVault &vault) : PotsReader(), mask(mask), vault(vault) {};
  void read_state() {
    if (STATE.button_mask == mask) {
      editing = true;
      update(&vault.n_places, 2, -7, 0);
      update(&vault.frac_beats, 1);
      update(&vault.frac_main, 2);
      update(&vault.prob, 3);
      vault.apply_edit(false);
    } else if (editing) {
      vault.apply_edit(true);
      disable();
      editing = false;
    }
  }
};

EuclideanButtons EBUTTONS1 = EuclideanButtons(0x3u, vault1);
EuclideanButtons EBUTTONS2 = EuclideanButtons(0xcu, vault2);

// reverb via array of values (could maybe save space with int16, but store extra bits to reduce noise)
// TODO - smear affects immediate output (it shouldn't)
template <int BITS> class Reverb {
private:
  static const uint max_size = 1 << BITS;
  int tape[max_size];
  uint write = 0;

  class TapeEMA : public EMA<int> {
  private:
    Reverb<BITS> *reverb;
  protected:
    int get_state() {return smear.next(reverb->tape[reverb->write]);}
    void set_state(int val) {reverb->tape[reverb->write] = val;}
  public:
    EMA<int> smear;
    TapeEMA(Reverb<BITS> *reverb, uint bits, uint num, uint xtra, uint sbits, uint snum, uint sxtra) 
    : reverb(reverb), smear(EMA<int>(sbits, snum, sxtra, 0)), EMA<int>(bits, num, xtra, 0) {};
  };

public:
  uint size = max_size;
  TapeEMA head;
  Reverb() : head(TapeEMA(this, 12, N12, 4, 12, N12, 4)) {};  // by default disabled
  int next(int val) {
    write = (write + 1) % max(1u, size);  // can't be "& mask" because size can vary
    return head.next(val);
  }
};

Reverb REVERB = Reverb<13>();

// post-process - outer two buttons
class PostButtons : public PotsReader {
public:
  PostButtons() = default;
  void read_state() {
    if (STATE.button_mask == 0x9) {
      update(&REVERB.size, 0, 1, 0);
      update(&REVERB.head.num, 1, 0, 1);
      update(&REVERB.head.smear.num, 1, 0, 2);
      update(&COMP_BITS, 0, -9, 3);
    } else {
      disable();
    }
  }
};

PostButtons PBUTTONS = PostButtons();

// use a timer to give regular sampling and (hopefully) reduce noise
SemaphoreHandle_t timer_semaphore;
esp_timer_handle_t timer_handle;

void IRAM_ATTR timer_callback(void*) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(timer_semaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// run the ui on other core
TaskHandle_t ui_handle = NULL;

void setup() {

  Serial.begin(115200);
  delay(100);
  Serial.println("hello world");

  // esp_log_level_set("*", ESP_LOG_NONE);

  for (Button& b : BUTTONS) b.init();
  for (Pot& p : POTS) p.init();

  dac_output_enable(DAC_CHAN_0);
  dac_output_voltage(DAC_CHAN_0, 128);  // Silencio inicial

  timer_semaphore = xSemaphoreCreateBinary();

  const esp_timer_create_args_t timer_args = {
    .callback = &timer_callback,
    .name = "high_freq_timer"
  };
  esp_timer_create(&timer_args, &timer_handle);
  esp_timer_start_periodic(timer_handle, TIMER_PERIOD_US);

  xTaskCreatePinnedToCore(&ui_loop, "UI Loop", 10000, NULL, 1, &ui_handle, 0);

  STATE.init();
}

// apply post-processing
uint post_process(int vol) {
  // apply compressor
  // int soft_clipped = vol;
  if (DBG_COMP) Serial.println(COMP_BITS);
  int sign = sgn(vol);
  uint absolute = abs(vol);
  for (uint i = (8 - COMP_BITS); i < 8; i++) {
    uint limit = 1 << i;
    if (absolute > limit) absolute = limit + (absolute - limit) / 2;
  }
  int soft_clipped = sign * static_cast<int>(absolute);
  // apply reverb
  int reverbed = REVERB.next(soft_clipped);
  if (DBG_REVERB && !random(DBG_LOTTERY)) Serial.printf("reverb %d -> %d\n", soft_clipped, reverbed);
  // hard clip
  int offset = reverbed + MAX7;
  int hard_clipped = max(0, min(static_cast<int>(MAX8), offset));
  if (DBG_VOLUME && !random(DBG_LOTTERY))
    Serial.printf("comp %d; vol %d; soft %d; hard %d\n", 
                  8 - COMP_BITS, vol, soft_clipped, hard_clipped);
  // TODO - why is reverb offset?!  the tape is an int, not a uint.  not even sure if order correct (before clipping?)
  return hard_clipped;
}

class Trigger {
private:
  EuclideanVault &vault;
  Euclidean *rhythm = nullptr;
  bool subdiv;
  enum Phase {Idle, Minor, Update};
  Phase phase = Idle;
  uint trigger = 0;
  void recalculate(uint ticks) {
    rhythm = vault.get();
    uint nv_interval = BEAT_SCALE / BPM;
    if (subdiv) nv_interval = (nv_interval * SUBDIVS[SUBDIV]) / 60;
    uint beat = 1 + (ticks / nv_interval);
    trigger = nv_interval * beat;
    if (trigger < ticks) trigger += nv_interval;  // TODO?
  }
public:
  Trigger(EuclideanVault &vault, bool subdiv) : vault(vault), subdiv(subdiv) {recalculate(0);}
  void on(uint ticks) {  // try to spread work across multiple ticks
    if (phase == Idle && ticks > trigger) {
      rhythm->on_beat(true);
      phase = Minor;
    } else if (phase == Minor) {
      rhythm->on_beat(false);
      phase = Update;
    } else if (phase == Update) {
      recalculate(ticks);
      phase = Idle;
    }
  }
};

// main loop on sound-generating core
void loop() {
  uint ticks = 0;
  unsigned long start = 0;
  EMA<unsigned long> gap = EMA<unsigned long>(4, 1, 3, 0);
  Trigger trigger1(vault1, false);
  Trigger trigger2(vault2, true);
  while (1) {
    if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdTRUE) {
      if (DBG_TIMING) {
        unsigned long latest = 0;
        unsigned long now = micros();
        if (start) latest = gap.next(now - start);
        start = now;
        if (!random(DBG_LOTTERY)) Serial.printf("%d %dms\n", ticks, latest);
      }
      if (ticks & 0x1) {
        trigger1.on(ticks);
      } else {
        trigger2.on(ticks);
      }
      ticks++;
      int vol = 0;
      for (Voice& voice : VOICES) vol += voice.output_12();
      // vol = VOICES[0].output_12();
      dac_output_voltage(DAC_CHAN_0, post_process(vol));
    }
  }
}

// main loop on other core for slow/ui operations
void ui_loop(void*) {
  while (1) {
    for (Button& b : BUTTONS) b.set_state();
    for (Pot& p : POTS) p.set_state();
    GBUTTONS.read_state();
    EBUTTONS1.read_state();
    EBUTTONS2.read_state();
    PBUTTONS.read_state();
    vTaskDelay(1);
  }
}
