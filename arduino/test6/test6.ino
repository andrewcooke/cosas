
#include <Arduino.h>
#include <atomic>
#include <array>
#include <numeric>
#include <mutex>
#include <thread>
#include "esp_timer.h"
#include "driver/dac.h"
#include "math.h"

const uint TIMER_PERIOD_US = 50;  // 40khz is 25 but we don't seem to get that high   see DBG_TIMING
const uint LOWEST_F_HZ = 10;  // with integer phase inc
const uint NSAMPLES = 1000000 / (LOWEST_F_HZ * TIMER_PERIOD_US);  // framing things this way gives constant frequency even if period changes
const uint BEAT_SCALE = 1000000 * 60 / (4 * TIMER_PERIOD_US);
volatile static uint BPM = 90;    // TODO - should be atomic, not volatile?
volatile static uint SWING = 0;
volatile static uint GAIN_BITS = 8;
volatile static uint COMP_BITS = 0;

const uint N6 = 1 << 6;
const uint MAX6 = N6 - 1;
const uint N7 = 1 << 7;
const uint MAX7 = N7 - 1;
const uint N8 = 1 << 8;
const uint MAX8 = N8 - 1;
const uint N11 = 1 << 11;
const uint MAX11 = N11 - 1;
const uint N12 = 1 << 12;
const uint MAX12 = N12 - 1;

const bool DBG_PATTERN = false;
const bool DBG_VOICE = false;
const bool DBG_LFSR = false;
const bool DBG_VOLUME = false;
const bool DBG_SWING = false;
const bool DBG_COMP = false;
const bool DBG_TIMING = true;

template <typename T> int sgn(T val) {return (T(0) < val) - (val < T(0));}

// a source of random bits
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
    // remove most of bias (shift to be around zero rather than all +ve)
    if (n > 0) bits -= 1 << (n - 1);  
    return bits;
  }
};

LFSR16 LFSR = LFSR16();

// wrapper for LEDs
class LEDs {
private:
  uint n_leds = 4;
  std::array<uint, 4> pins = { 23, 32, 5, 2 };
public:
  void init() {
    for (uint i = 0; i < n_leds; i++) pinMode(pins[i], OUTPUT);
    all_off();
  }
  void set(uint led, uint level) {analogWrite(pins[led], level);}
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

// central location for coordinating (utliple) button presses
// also wraps leds
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
  void pots(float frac) {
    frac *= 4;
    for (uint i = 0; i < 4; i++) {
      if (frac > 1) {leds.on(i); frac -= 1;}
      else if (frac > 0) {leds.set(i, static_cast<uint>(frac * MAX8)); frac = 0;}
      else leds.off(i);
    }
  }
};

CentralState STATE = CentralState();

// lookup table for sine
class Sine {
private:
  std::array<uint, NSAMPLES / 4> table;
public:
  Sine() {for (uint i = 0; i < NSAMPLES / 4; i++) table[i] = MAX12 * sin(2 * PI * i / NSAMPLES);}
  int operator()(uint amp, int phase) {
    int sign = 1;
    if (phase >= NSAMPLES / 2) {
      phase = NSAMPLES - phase;
      sign = -1;
    };
    if (phase >= NSAMPLES / 4) phase = (NSAMPLES / 2) - phase;
    // i had to draw this out on a piece of paper before i was convinced
    if (phase == NSAMPLES / 4) return sign * amp;
    else return sign * ((amp * table[phase]) >> 12);
  }
};

Sine SINE = Sine();

// linear decay of sine with fm based on time or noise
class Voice {
private:
  uint idx;
  uint time = 0;
  int phase = 0;
public:
  volatile uint amp;    // 12 bits
  volatile uint freq;   // 12 bits
  volatile uint durn;   // 12 bits
  volatile uint fm;  // 12 bits
  Voice(uint idx, uint amp, uint freq, uint durn, uint fm)
    : idx(idx), amp(amp), freq(freq), durn(durn), fm(fm){};
  void trigger() {
    time = 0;
    phase = 0;
  }
  int output_12() {
    static int noise = 0;
    uint freq_scaled = freq >> 4;
    freq_scaled = ((freq_scaled * freq_scaled) >> 8) & 0xff0;  // quantisation?
    uint fm_scaled = fm >> 8;
    fm_scaled = (fm_scaled * fm_scaled) >> 3;
    if (fm_scaled < 6) phase += freq_scaled - (time >> (7 + fm_scaled));
    else {
      if (time & 0x1) noise = LFSR.n_bits(fm_scaled - 6);  // de-alias
      phase += freq_scaled + noise;
    }
    while (phase < 0) phase += NSAMPLES;
    while (phase >= NSAMPLES) phase -= NSAMPLES;
    uint durn_scaled = durn << 2;
    if (time == durn_scaled) STATE.voice(idx, false);
    if (time > durn_scaled) return 0;
    uint amp_scaled = amp >> 1;  // because signed output is 12 bits
    amp_scaled *= (durn_scaled - time) / static_cast<float>(durn_scaled);
    time++;
    return SINE(amp_scaled, phase);
  }
};

std::array<Voice, 4> VOICES = {Voice(0, MAX12, 160, 1200, 0),
                               Voice(1, MAX12, 240, 1000, 240),
                               Voice(2, MAX12, 213, 1300, 150),
                               Voice(3, MAX12, 320,  900, 240)};

// standard euclidean pattern
// TODO - add variations (biased towards beats with largest errors)
class Euclidean {
private:
  uint n_places;
  uint n_beats;
  float frac_main;
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
  // n_beats must be <= n_places
  Euclidean(uint n_places, uint n_beats, float frac_main, uint voice)
    : n_places(n_places), n_beats(n_beats), frac_main(frac_main), voice(voice) {
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
    n_main = max(1u, min(static_cast<uint>(round(n_beats * frac_main)), n_beats - 1));
    is_main.resize(n_beats, false);
    for (uint i = 0; i < n_main; i++) is_main[index_by_error[i]] = true;
    index_by_place.resize(n_places, -1);
    for (uint i = 0; i < n_beats; i++) index_by_place[place[i]] = i;
  };
  Euclidean() : Euclidean(2, 1, 0.5, 0){};  // used only as temp value in arrays
  bool operator==(const Euclidean other) {
    return other.n_places == n_places && other.n_beats == n_beats && other.n_main == n_main && other.voice == voice;
  }
  void on_beat(bool main) {
    int beat = index_by_place[current_place];
    if (beat != -1 && is_main[beat] == main) {
      trigger(main ? 0 : 1);
      if (DBG_PATTERN) {
        Serial.print(voice); Serial.print(": "); Serial.print(current_place); Serial.print("/"); Serial.print(n_places);
        Serial.print(" "); Serial.print(beat); Serial.print(" ("); Serial.print(main); Serial.println(")");
      }
    }
    if (main) {  // minor is done before main
      if (++current_place == n_places) current_place = 0;
    }
  }
};

// ema smooth pot values
class Pot {
private:
  static const uint ema_bits = 3;
  static const uint ema_num = 1;
  static const uint ema_denom = 1 << ema_bits;
  static const uint ema_xbits = 3;
  uint pin;
  uint ema = 0;
public:
  uint state = 0;
  Pot(uint pin) : pin(pin) {};
  void init() {pinMode(pin, INPUT);}
  void set_state() {
    uint tmp = analogRead(pin);
    ema = (ema * (ema_denom - ema_num) + (tmp << ema_xbits) * ema_num) >> ema_bits;
    state = MAX12 - (ema >> ema_xbits) - 1;  // inverted and hack to zero
  }
};

// the smoothed pots
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

// subclass button to edit voice parameters
class VoiceButton : public Button {
private:
  static const uint thresh = 10;
  Voice& voice;
  std::array<bool, 4> enabled = { false, false, false, false };
  void take_action() {
    if (pressed && STATE.n_buttons == 1) {
      update(&voice.amp, 0);
      update(&voice.freq, 1);
      update(&voice.durn, 2);
      update(&voice.fm, 3);
      if (DBG_VOICE) {
        Serial.print("a "); Serial.print(voice.amp); Serial.print(", f "); Serial.print(voice.freq);
        Serial.print(", d "); Serial.print(voice.durn); Serial.print(", n "); Serial.println(voice.fm);
      }
    } else if (changed && !pressed) {
      Serial.print("Voice("); Serial.print(idx); Serial.print(","); Serial.print(voice.amp); Serial.print(","); Serial.print(voice.freq);
      Serial.print(","); Serial.print(voice.durn); Serial.print(","); Serial.print(voice.fm); Serial.println(")");
      std::fill(std::begin(enabled), std::end(enabled), false);
    }
  }
  void update(volatile uint* voice, uint pot) {
    if (!enabled[pot] && abs(static_cast<int>(*voice) - static_cast<int>(POTS[pot].state)) < thresh) {
      enabled[pot] = true;
      STATE.pot(pot);
    }
    if (enabled[pot]) *voice = POTS[pot].state;
  }
public:
  VoiceButton(uint idx, uint pin, Voice& voice) : Button(idx, pin), voice(voice){};
};

std::array<VoiceButton, 4> BUTTONS = { VoiceButton(0, 18, VOICES[0]),
                                       VoiceButton(1, 4, VOICES[1]),
                                       VoiceButton(2, 15, VOICES[2]),
                                       VoiceButton(3, 19, VOICES[3]) };

// global parameters - hold down middle two buttons
class GlobalButtons {
private:
  const uint thresh = 10;
  std::array<bool, 4> enabled = { false, false, false, false };
  void update(volatile uint* param, uint zero, uint bits, uint pot) {
    if (!enabled[pot] && abs(static_cast<int>((*param - zero) << bits) - static_cast<int>(POTS[pot].state)) < thresh) {
      enabled[pot] = true;
      STATE.pot(pot);
    }
    if (enabled[pot]) *param = zero + (POTS[pot].state >> bits);
  }
public:
  GlobalButtons() = default;
  void set_state() {
    if (STATE.button_mask == 0x6) {
      update(&BPM, 30, 5, 0);
      update(&SWING, 0, 0, 1);
      // both inverted to make pots work in right direcn
      update(&GAIN_BITS, 0, 9, 2);
      update(&COMP_BITS, 0, 9, 3);
    } else {
      std::fill(std::begin(enabled), std::end(enabled), false);
    }
  }
};

GlobalButtons GBUTTONS = GlobalButtons();

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
  uint n_beats;
  float frac_main;
  EuclideanVault(uint n_places, uint n_beats, float frac_main, uint voice)
    : n_places(n_places), n_beats(n_beats), frac_main(frac_main), voice(voice) {
    euclideans[0] = Euclidean(n_places, n_beats, frac_main, voice);
    euclideans[1] = Euclidean(n_places, n_beats, frac_main, voice);
  }
  void apply_edit(bool dump) {
    std::lock_guard<std::mutex> lock(access);
    n_beats = min(n_beats, n_places);
    Euclidean candidate = Euclidean(n_places, n_beats, frac_main, voice);;
    if (euclideans[updated ? next : current] != candidate) {
      Serial.print("Euclidean("); Serial.print(n_places); Serial.print(","); Serial.print(n_beats); 
      Serial.print(","); Serial.print(frac_main); Serial.print(","); Serial.print("voice"); Serial.println(")");
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

EuclideanVault vault1 = EuclideanVault(16, 7, 0.33, 0);
EuclideanVault vault2 = EuclideanVault(25, 13, 0.25, 2);

// edit patterns - hold down left or right two buttons (mask)
class EuclideanButtons {
private:
  const uint thresh = 10;
  const std::array<uint, 4> prime = {2, 3, 5, 7};
  uint mask;
  EuclideanVault &vault;
  std::array<bool, 4> enabled = { false, false, false, false };
  bool editing = false;
  void update(uint* param, uint zero, uint bits, uint pot) {
    enabled[pot] = enabled[pot] || abs(static_cast<int>((*param - zero) << bits) - static_cast<int>(POTS[pot].state)) < thresh;
    if (enabled[pot]) {
      *param = zero + (POTS[pot].state >> bits);
      STATE.pot_clear();
      for (uint i = 0; i < 4; i++) if (*param % prime[i] == 0) STATE.pot(i);
    }
  }
  void update_float(float* param, uint pot) {
    enabled[pot] = enabled[pot] || abs(*param * MAX12 - POTS[pot].state) < thresh;
    if (enabled[pot]) {
      *param = static_cast<float>(POTS[pot].state) / MAX12;
      STATE.pots(*param);
    }
  }
public:
  EuclideanButtons(uint mask, EuclideanVault &vault) : mask(mask), vault(vault) {};
  void set_state() {
    if (STATE.button_mask == mask) {
      if (STATE.button_mask_changed) editing = true;
      update(&vault.n_places, 2, 6, 0);
      update(&vault.n_beats, 2, 6, 1);
      update_float(&vault.frac_main, 2);
      vault.apply_edit(false);
    } else if (editing) {
      vault.apply_edit(true);
      std::fill(std::begin(enabled), std::end(enabled), false);
      editing = false;
    }
  }
};

EuclideanButtons EBUTTONS1 = EuclideanButtons(0x3u, vault1);
EuclideanButtons EBUTTONS2 = EuclideanButtons(0xcu, vault2);

// regular sampling
SemaphoreHandle_t timer_semaphore;
esp_timer_handle_t timer_handle;

void IRAM_ATTR timer_callback(void*) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(timer_semaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// ui on other core
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

uint scale_and_clip(int vol) {
  int scaled = vol >> (8 - GAIN_BITS);
  int sign = sgn(scaled);
  int absolute = abs(scaled);
  int soft_clipped = absolute;
  if (DBG_COMP) Serial.println(COMP_BITS);
  for (uint i = (8 - COMP_BITS); i < 8; i++) {
    uint limit = 1 << i;
    if (soft_clipped > limit) soft_clipped = limit + (soft_clipped - limit) / 2;
  }
  soft_clipped *= sign;
  int offset = soft_clipped + MAX7;
  int hard_clipped = max(0, min(static_cast<int>(MAX8), offset));
  if (DBG_VOLUME && !random(10000)) {
    Serial.print("G "); Serial.print(8 - GAIN_BITS); Serial.print(", C "); Serial.print(8 - COMP_BITS); 
    Serial.print(", v "); Serial.print(vol); Serial.print(", s "); Serial.print(scaled);
    Serial.print(", sc "); Serial.print(soft_clipped); Serial.print(", hc "); Serial.println(hard_clipped);
  }
  return hard_clipped;
}

// semaphore gives regular sampleas as long as no single iteration takes too long
void loop() {
  unsigned long start = 0;
  uint accum = 0;
  uint tick = 0;
  uint beat = BEAT_SCALE / BPM;  // if it's 4/4 time
  uint swing = (SWING * beat) >> 12;
  Euclidean* rhythm1 = nullptr;
  Euclidean* rhythm2 = nullptr;
  while (1) {
    if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdTRUE) {
      if (DBG_TIMING) {
        unsigned long now = micros();
        if (start) accum = (15 * accum + ((now - start) << 3)) >> 4;
        start = now;
        if (!random(100000)) Serial.println(accum >> 3);
      }
      // spread out the load
      if (tick == 0) {  // update on first beat TODO - change to ends of cycles
        rhythm1 = vault1.get();
        rhythm2 = vault2.get();
      } else if (tick == 1) {
        beat = BEAT_SCALE / BPM;
        swing = (SWING * beat) >> 12;
        if (DBG_SWING) {Serial.print(swing); Serial.print("/"); Serial.println(beat);}
      } else if (tick == 2) {
        rhythm1->on_beat(true);
      } else if (tick == 3) {
        rhythm2->on_beat(true);
      }
      uint offset = beat - tick;
      if (offset == swing + 2) {
        rhythm1->on_beat(false);
      } else if (offset == swing + 1) {
        rhythm2->on_beat(false);
      }
      int vol = 0;
      for (Voice& voice : VOICES) vol += voice.output_12();
      dac_output_voltage(DAC_CHAN_0, scale_and_clip(vol));
      if (++tick == beat) tick = 0;
    }
  }
}

// runs on other core so can do slow operations
void ui_loop(void*) {
  while (1) {
    for (Button& b : BUTTONS) b.set_state();
    for (Pot& p : POTS) p.set_state();
    GBUTTONS.set_state();
    EBUTTONS1.set_state();
    EBUTTONS2.set_state();
    vTaskDelay(1);
  }
}
