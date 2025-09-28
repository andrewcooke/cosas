
#include <Arduino.h>
#include <atomic>
#include <array> 
#include <numeric>
#include "esp_timer.h"
#include "driver/dac.h"
#include "math.h"

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
  uint n_bits(uint n) {
    uint bits = 0;
    while (n) {
      bits = bits << 1 | next();
      n--;
    }
    return bits;
  }
};

class LEDs {
private:
  uint n_leds = 4;
  std::array<uint, 4> pins = {23, 32, 5, 2};
public:
  void init() {for (uint i = 0; i < n_leds; i++) pinMode(pins[i], OUTPUT); all_off();}
  void set(uint led, uint level) {analogWrite(pins[led], level);}
  void on(uint led) {digitalWrite(pins[led], HIGH);}
  void off(uint led) {digitalWrite(pins[led], LOW);}
  void all_on() {for (uint i = 0; i < n_leds; i++) on(i);}
  void all_off() {for (uint i = 0; i < n_leds; i++) off(i);}
  void start_up() {all_on(); delay(1000); all_off(); delay(1000); all_on(); delay(1000); all_off();}
};

LEDs LEDS = LEDs();

const uint NVOICES = 4;
std::array<std::atomic<uint>, NVOICES> TIME = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> PHASE = {0, 0, 0, 0};

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
  void trigger(uint voice) {
    TIME[voice] = 0;
    PHASE[voice] = 0;  // really?
    LEDS.on(voice);
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
    if (beat != -1) trigger(is_main[beat] ? voice : voice+1);
    if (++current_place == n_places) current_place = 0;
  }
};

const uint TIMER_PERIOD_US = 50;  // about as fast as we can go :(
const uint NSAMPLES = 1000000 / TIMER_PERIOD_US;
const uint BPM = 90;

const uint NCTRLS = 4;
const uint POT[NCTRLS] = {13, 14, 27, 12};
const uint BTN[NCTRLS] = {18, 4, 15, 19};

uint BTN_TMP[NCTRLS] = {0};
uint BTN_STATE[NCTRLS] = {0};
uint BTN_CHG[NCTRLS] = {0};
uint BTN_LAST[NCTRLS] = {0};
const uint BTN_DEBOUNCE = 50;
uint POT_EMA[NCTRLS] = {0};
uint POT_STATE[NCTRLS] = {0};
const uint POT_EMA_NUM = 1;
const uint POT_EMA_BITS = 3;
const uint POT_EMA_DENOM = 1 << POT_EMA_BITS;
const uint POT_EMA_XBITS = 3;

const uint N7 = 1 << 7;
const uint MAX7 = N7 - 1;
const uint N8 = 1 << 8;
const uint MAX8 = N8 - 1;
const uint N11 = 1 << 11;
const uint MAX11 = N11 - 1;
const uint N12 = 1 << 12;
const uint MAX12 = N12 - 1;

// these are all 12 bits
std::array<std::atomic<uint>, NVOICES> AMP = {MAX12, MAX12, MAX12, MAX12};
std::array<std::atomic<uint>, NVOICES> FREQ = {1600,  2400, 2133, 3200};
std::array<std::atomic<uint>, NVOICES> DURN = {1200,  1000, 1300, 900};
std::array<std::atomic<uint>, NVOICES> NOISE = {0,    2400, 1500, 2400};

std::array<uint, NSAMPLES / 4> QSINE;

Lfsr16 lfsr = Lfsr16();

Euclidean rhythm1 = Euclidean(16,  7, 0.33, 0);
Euclidean rhythm2 = Euclidean(25, 13, 0.25, 2);

uint tick = 0;

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

  esp_log_level_set("*", ESP_LOG_NONE);

  LEDS.init();

  for (uint i = 0; i < NCTRLS; i++) {
    pinMode(POT[i], INPUT);
    pinMode(BTN[i], INPUT_PULLUP);
  }
  
  dac_output_enable(DAC_CHAN_0);
  dac_output_voltage(DAC_CHAN_0, 128); // Silencio inicial
  
  timer_semaphore = xSemaphoreCreateBinary();

  const esp_timer_create_args_t timer_args = {
    .callback = &timer_callback,
    .name = "high_freq_timer"
  };
  esp_timer_create(&timer_args, &timer_handle);
  esp_timer_start_periodic(timer_handle, TIMER_PERIOD_US);

  for (uint i = 0; i < NSAMPLES / 4; i++) QSINE[i] = MAX12 * sin(2 * PI * i / NSAMPLES);

  xTaskCreatePinnedToCore(&ui_loop, "UI Loop", 10000, NULL, 1, &ui_handle, 0);

  LEDS.start_up();
}

void loop() {
  uint beat = (NSAMPLES * 60) / (BPM * 4);  // if it's 4/4 time
  while (1) {
    if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdTRUE) {
      if (tick == 0) {
        LEDS.all_off();
        rhythm1.on_beat();
        rhythm2.on_beat();
      }
      int vol = 0;
      for (uint i = 0; i < NVOICES; i++) vol += calc_output_12(i);
      vol = vol / 64 + MAX7;
      vol = vol > MAX8 ? MAX8 : vol;
      vol = vol < 0 ? 0 : vol;
      dac_output_voltage(DAC_CHAN_0, vol);
      if (++tick == beat) tick = 0;
    }
  }
}

void ui_loop(void*) {
  while (1) {
    set_ui_state();
    apply_ui_state();
    vTaskDelay(1);
  }
}

void set_ui_state() {
  for (uint i = 0; i < NCTRLS; i++) {
    BTN_CHG[i] = 0;
    uint btn_tmp = 1 - digitalRead(BTN[i]);
    uint now = millis();
    if (BTN_LAST[i] == 0 || BTN_TMP[i] != btn_tmp) {
      BTN_TMP[i] = btn_tmp;
      BTN_LAST[i] = now;
    } else if (now - BTN_LAST[i] > BTN_DEBOUNCE && BTN_TMP[i] != BTN_STATE[i]) {
      BTN_STATE[i] = BTN_TMP[i];
      BTN_CHG[i] = 1;
    }
    uint pot_tmp = analogRead(POT[i]);
    POT_EMA[i] = (POT_EMA[i] * (POT_EMA_DENOM - POT_EMA_NUM) + (pot_tmp << POT_EMA_XBITS) * POT_EMA_NUM) >> POT_EMA_BITS;
    POT_STATE[i] = MAX12 - (POT_EMA[i] >> POT_EMA_XBITS);
  }
}

void apply_ui_state() {
  // for (uint i = 0; i < NCTRLS; i++) Serial.print(BTN_STATE[i]);
  // Serial.println(" buttons");
  set_pots();
}

void set_pots() {
  AMP[0] = POT_STATE[0];
  FREQ[0] = POT_STATE[1];
  DURN[0] = POT_STATE[2];
  NOISE[0] = POT_STATE[3];
}

int calc_sine(uint amp, int phase) {
  int sign = 1;
  if (phase > NSAMPLES / 2) {
    phase = NSAMPLES - phase;
    sign = -1;
  };
  if (phase > NSAMPLES / 4) phase = (NSAMPLES / 2) - phase;
  return sign * ((amp * QSINE[phase]) >> 12);
}

int calc_output_12(uint voice) {
  uint time = TIME[voice]++;
  uint durn = DURN[voice] << 2;  // arbitrary scale
  if (time > durn) return 0;
  uint freq = FREQ[voice] >> 4;
  uint noise = NOISE[voice] >> 8;
  uint phase = PHASE[voice];
  phase = phase + freq + lfsr.n_bits(noise);
  while (phase > NSAMPLES) phase -= NSAMPLES;
  PHASE[voice] = phase;
  uint amp = (durn - time) * (AMP[voice] >> 1) / static_cast<float>(durn);  // >> 1 because signed
  int output = calc_sine(amp, phase);
  return output;
}
