
#include <Arduino.h>
#include <atomic>
#include <array> 
#include <numeric>
#include "esp_timer.h"
#include "driver/dac.h"
#include "math.h"

const uint TIMER_PERIOD_US = 50;  // about as fast as we can go :(
const uint NSAMPLES = 1000000 / TIMER_PERIOD_US;
const uint BPM = 90;

const uint N7 = 1 << 7;
const uint MAX7 = N7 - 1;
const uint N8 = 1 << 8;
const uint MAX8 = N8 - 1;
const uint N11 = 1 << 11;
const uint MAX11 = N11 - 1;
const uint N12 = 1 << 12;
const uint MAX12 = N12 - 1;

class Lfsr16 {
private:
  uint16_t state;
public:
  Lfsr16(uint16_t seed = 0xACE1) : state(seed) {}
  uint next() {
    uint16_t lsb = state & 0x1;
    state >>= 1;
    if (lsb) state ^= 0xB400;
    return lsb;
  }
  int n_bits(uint n) {
    int bits = 0;
    while (n) {
      bits = bits << 1 | next();
      n--;
    }
    if (n > 0) bits -= 1 << (n - 1);
    return bits;
  }
};

Lfsr16 LFSR = Lfsr16();

class LEDs {
private:
  uint n_leds = 4;
  std::array<uint, 4> pins = {23, 32, 5, 2};
public:
  void init() {for (uint i = 0; i < n_leds; i++) pinMode(pins[i], OUTPUT); all_off();}
  void set(uint led, uint level) {analogWrite(pins[led], level);}
  void on(uint led) {digitalWrite(pins[led], HIGH);}
  void on(uint led, bool on) {digitalWrite(pins[led], on ? HIGH : LOW);}
  void off(uint led) {digitalWrite(pins[led], LOW);}
  void all_on() {for (uint i = 0; i < n_leds; i++) on(i);}
  void all_off() {for (uint i = 0; i < n_leds; i++) off(i);}
  void start_up() {all_on(); delay(100); all_off(); delay(100); all_on(); delay(1000); all_off();}
};

class CentralState {
private:
  LEDs leds = LEDs();
public:
  uint button_mask = 0;
  uint n_buttons = 0;
  CentralState() = default;
  void init() {leds.init(); leds.start_up();}
  void button(uint idx, bool on) {
    uint mask = 1 << idx;
    if (on) button_mask |= mask;
    else button_mask &= ~mask;
    Serial.print("button "); Serial.print(idx); Serial.print(" "); Serial.print(on); Serial.print(" "); Serial.println(button_mask);
    n_buttons = std::popcount(button_mask);
    leds.all_off();
  }
  void voice(uint idx, bool on) {if (! button_mask) leds.on(idx, on);}
  void pot(uint idx) {leds.on(idx);}
};

CentralState STATE = CentralState();

class Sine {
private:
  std::array<uint, NSAMPLES / 4> table;
public:
  Sine() {for (uint i = 0; i < NSAMPLES / 4; i++) table[i] = MAX12 * sin(2 * PI * i / NSAMPLES);}
  int operator()(uint amp, int phase) {
    int sign = 1;
    if (phase > NSAMPLES / 2) {
      phase = NSAMPLES - phase;
      sign = -1;
    };
    if (phase > NSAMPLES / 4) phase = (NSAMPLES / 2) - phase;
    return sign * ((amp * table[phase]) >> 12);
  }
};

Sine SINE = Sine();

class Voice {
private:
  uint idx;
  uint time = 0;
  uint phase = 0;
public:
  uint amp;    // 12 bits
  uint freq;   // 12 bits
  uint durn;   // 12 bits
  uint noise;  // 12 bits
  Voice(uint idx, uint amp, uint freq, uint durn, uint noise) : idx(idx), amp(amp), freq(freq), durn(durn), noise(noise) {};
  void trigger() {time = 0; phase = 0;}
  int output_12() {
    time++;
    uint freq_scaled = freq >> 4;
    uint noise_scaled = noise >> 8;
    phase += freq_scaled + LFSR.n_bits(noise_scaled);
    while (phase > NSAMPLES) phase -= NSAMPLES;
    uint durn_scaled = durn << 2;
    if (time == durn_scaled) STATE.voice(idx, false);
    if (time > durn_scaled) return 0;
    uint amp_scaled = amp >> 1;  // because signed output is 12 bits
    amp_scaled *= (durn_scaled - time) / static_cast<float>(durn_scaled);
    return SINE(amp_scaled, phase);
  }
};

std::array<Voice, 4> VOICES = {Voice(0, MAX12, 1600, 1200, 0),
                               Voice(1, MAX12, 2400, 1000, 2400),
                               Voice(2, MAX12, 2133, 1300, 1500),
                               Voice(3, MAX12, 3200,  900, 2400)};

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
    VOICES[voice+delta].trigger();
    STATE.voice(voice + delta, true);
  }
public:
  // n_beats must be <= n_places
  Euclidean(uint n_places, uint n_beats, float frac_main, uint voice) : n_places(n_places), n_beats(n_beats), frac_main(frac_main), voice(voice) {
    for (uint i = 0; i < n_beats; i++) {
      float x = n_places * i / static_cast<float>(n_beats);
      place.push_back(round(x));
      error.push_back(x - place[i]);
    };
    index_by_error.resize(n_beats, 0);
    std::iota(index_by_error.begin(), index_by_error.end(), 0);
    std::sort(index_by_error.begin(), index_by_error.end(), [this](int i, int j) {return abs(this->error[i]) < abs(this->error[j]);});
    n_main = max(1u, min(static_cast<uint>(round(n_beats * frac_main)), n_beats - 1));
    is_main.resize(n_beats, false);
    for (uint i = 0; i < n_main; i++) is_main[index_by_error[i]] = true;
    index_by_place.resize(n_places, -1);
    for (uint i = 0; i < n_beats; i++) index_by_place[place[i]] = i;
  };
  void on_beat() {
    int beat = index_by_place[current_place];
    if (beat != -1) trigger(is_main[beat] ? 0 : 1);
    if (++current_place == n_places) current_place = 0;
  }
};

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

std::array<Pot, 4> POTS = {Pot(13), Pot(14), Pot(27), Pot(12)};

class Button {
private:
  static const uint debounce = 50;
  uint idx;
  uint pin;
  bool tmp_state = false;
  uint tmp_change = 0;
  virtual void take_action() = 0;
public:
  bool state = false;
  bool changed = false;
  Button(uint idx, uint pin) : idx(idx), pin(pin) {};
  void init() {pinMode(pin, INPUT_PULLUP);}
  void set_state() {
    changed = false;
    bool current = !digitalRead(pin);  // inverted
    uint now = millis();
    if (tmp_change == 0 || current != tmp_state) {
      tmp_state = current;
      tmp_change = now;
    } else if (now - tmp_change > debounce && tmp_state != state) {
      state = tmp_state;
      changed = true;
      STATE.button(idx, state);
    }
    take_action();
  }
};

class VoiceButton : public Button {
private:
  static const uint thresh = 10;
  Voice& voice;
  std::array<bool, VOICES.size()> enabled = {false, false, false, false};
  void take_action() {
    if (state && STATE.n_buttons == 1) {
      update(&voice.amp, 0);
      update(&voice.freq, 1);
      update(&voice.durn, 2);
      update(&voice.noise, 3);
    } else if (changed && !state) {
      std::fill(std::begin(enabled), std::end(enabled), false);
    }
  }
  void update(uint* voice, uint pot) {
    if (!enabled[pot] && abs(static_cast<int>(*voice) - static_cast<int>(POTS[pot].state)) < thresh) {
      enabled[pot] = true;
      STATE.pot(pot);
    }
    if (enabled[pot]) *voice = POTS[pot].state;
  }
public:
  VoiceButton(uint idx, uint pin, Voice& voice) : Button(idx, pin), voice(voice) {};
};

std::array<VoiceButton, 4> BUTTONS = {VoiceButton(0, 18, VOICES[0]), 
                                      VoiceButton(1, 4,  VOICES[1]), 
                                      VoiceButton(2, 15, VOICES[2]), 
                                      VoiceButton(3, 19, VOICES[3])};

Euclidean rhythm1 = Euclidean(16,  7, 0.33, 0);
Euclidean rhythm2 = Euclidean(25, 13, 0.25, 2);

SemaphoreHandle_t timer_semaphore;
esp_timer_handle_t timer_handle;

void IRAM_ATTR timer_callback(void*) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(timer_semaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

TaskHandle_t ui_handle = NULL;

void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("hello world");

  // esp_log_level_set("*", ESP_LOG_NONE);

  for (Button& b: BUTTONS) b.init();
  for (Pot& p: POTS) p.init();
  
  dac_output_enable(DAC_CHAN_0);
  dac_output_voltage(DAC_CHAN_0, 128); // Silencio inicial
  
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

void loop() {
  uint tick = 0;
  uint beat = (NSAMPLES * 60) / (BPM * 4);  // if it's 4/4 time
  while (1) {
    if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdTRUE) {
      if (tick == 0) {
        rhythm1.on_beat();
        rhythm2.on_beat();
      }
      int vol = 0;
      for (Voice& voice: VOICES) vol += voice.output_12();
      vol = vol / 64 + MAX7;
      vol = min(static_cast<int>(MAX8), max(0, vol));
      dac_output_voltage(DAC_CHAN_0, vol);
      if (++tick == beat) tick = 0;
    }
  }
}

void ui_loop(void*) {
  while (1) {
    set_ui_state();
    vTaskDelay(1);
  }
}

void set_ui_state() {
  for (Button& b: BUTTONS) b.set_state();
  for (Pot& p: POTS) p.set_state();
}
