
#include <Arduino.h>
#include <atomic>
#include <array> 
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

const uint TIMER_PERIOD_US = 50;  // about as fast as we can go :(
const uint NSAMPLES = 1000000 / TIMER_PERIOD_US;

const uint NCTRLS = 4;
const uint POT[NCTRLS] = {13, 14, 27, 12};
const uint BTN[NCTRLS] = {18, 4, 15, 19};
const uint LED[NCTRLS] = {23, 32, 5, 2};

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

const uint NVOICES = NCTRLS;
// these are all 12 bits
std::array<std::atomic<uint>, NVOICES> AMP = {MAX12, MAX12, MAX12, MAX12};
std::array<std::atomic<uint>, NVOICES> DURN = {1000, 2000, 3000, 4000};
std::array<std::atomic<uint>, NVOICES> FREQ = {1000, 2000, 3000, 4000};
std::array<std::atomic<uint>, NVOICES> NOISE = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> TIME = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> PHASE = {0, 0, 0, 0};

Lfsr16 lfsr = Lfsr16();

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

  for (uint i = 0; i < NCTRLS; i++) {
    pinMode(POT[i], INPUT);
    pinMode(BTN[i], INPUT_PULLUP);
    pinMode(LED[i], OUTPUT);
    digitalWrite(LED[i], LOW);
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

  xTaskCreatePinnedToCore(&ui_loop, "UI Loop", 10000, NULL, 1, &ui_handle, 0);

  flash_leds();
}

void flash_leds() {
  for (uint j = 0; j < 3; j++) {
    for (uint i = 0; i < 4; i++) digitalWrite(LED[i], HIGH);
    delay(100);
    for (uint i = 0; i < 4; i++) digitalWrite(LED[i], LOW);
    delay(100);
  }
}

void loop() {
  while (1) {
    if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdTRUE) {
      int vol = 0;
      vol = calc_output_12(0);
      vol = vol / 16 + MAX7;  // 12 -> 8 bits
      vol = vol > MAX8 ? MAX8 : vol;
      vol = vol < 0 ? 0 : vol;
      dac_output_voltage(DAC_CHAN_0, vol);
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
    POT_STATE[i] = MAX12 - (POT_EMA[i] >> POT_EMA_XBITS) - 1;  // fudge factor to get zero
  }
}

void apply_ui_state() {
  // for (uint i = 0; i < NCTRLS; i++) Serial.print(BTN_STATE[i]);
  // Serial.println(" buttons");
  set_pots();
  if (BTN_STATE[0] && BTN_CHG[0]) reset_time();
}

void set_pots() {
  AMP[0] = POT_STATE[0]; analogWrite(LED[0], POT_STATE[0] >> 4);
  FREQ[0] = POT_STATE[1]; analogWrite(LED[1], POT_STATE[1] >> 4);
  DURN[0] = POT_STATE[2]; analogWrite(LED[2], POT_STATE[2] >> 4);
  NOISE[0] = POT_STATE[3]; analogWrite(LED[3], POT_STATE[3] >> 4);
}

void reset_time() {
  TIME[0] = 0;
  PHASE[0] = 0;
  Serial.print("amp "); Serial.println(AMP[0]);
  Serial.print("freq "); Serial.println(FREQ[0]);
  Serial.print("durn "); Serial.println(DURN[0]);
  Serial.print("noise "); Serial.println(NOISE[0]);
}

int calc_output_12(uint voice) {
  uint time = TIME[voice]++;
  uint durn = DURN[voice] << 2;  // arbitrary scale
  if (time > durn) return 0;
  uint freq = FREQ[voice] >> 4;
  uint noise_bits = NOISE[voice] >> 8;
  int noise = lfsr.n_bits(noise_bits);
  if (noise_bits > 1) noise -= 1 << (noise_bits - 1);
  uint phase = PHASE[voice];
  phase = (phase + freq + noise) % NSAMPLES;
  PHASE[voice] = phase;
  float amp = (durn - time) * (AMP[voice] >> 1) / static_cast<float>(durn);  // >> 1 because signed
  int output = static_cast<int>(amp * sin(2 * PI * phase / static_cast<float>(NSAMPLES)));
  return output;
}

