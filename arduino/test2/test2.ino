
#include <Arduino.h>
#include <atomic>
#include <array> 
#include "esp_timer.h"
#include "driver/dac.h"
#include "math.h"

//#define TIMER_PERIOD_US 21  // 48kHz is 20.83us
#define TIMER_PERIOD_US 50  // about as fast as we can go :(

const uint NCTRLS = 4;
const uint POT[NCTRLS] = {13, 14, 12, 27};
const uint BTN[NCTRLS] = {18, 19, 15, 4};
const uint LED[NCTRLS] = {23, 32, 5, 2};
uint BTN_TMP[NCTRLS] = {0};
uint BTN_STATE[NCTRLS] = {0};
uint BTN_LAST[NCTRLS] = {0};
uint BTN_DEBOUNCE = 50;
uint POT_EMA[NCTRLS] = {0};
uint POT_STATE[NCTRLS] = {0};
const uint POT_EMA_NUM = 1;
const uint POT_EMA_BITS = 3;
const uint POT_EMA_DENOM = 1 << (POT_EMA_BITS - 1);
const uint POT_EMA_XBITS = 3;

const uint NVOICES = NCTRLS;
std::array<std::atomic<uint>, NVOICES> AMP = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> FREQ = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> NOISE = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> DURN = {0, 0, 0, 0};
std::array<std::atomic<uint>, NVOICES> TICK = {0, 0, 0, 0};

SemaphoreHandle_t timer_semaphore;
esp_timer_handle_t high_freq_timer;

void IRAM_ATTR timer_callback(void*) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(timer_semaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

TaskHandle_t uiTask = NULL;

void setup() {

  Serial.begin(115200);

  esp_log_level_set("*", ESP_LOG_NONE);

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
  esp_timer_create(&timer_args, &high_freq_timer);
  esp_timer_start_periodic(high_freq_timer, TIMER_PERIOD_US);

  xTaskCreatePinnedToCore(uiLoop, "UI Loop", 1000000, NULL, 1, &uiTask, 0);

  for (uint j = 0; j < 3; j++) {
    for (uint i = 0; i < 4; i++) digitalWrite(LED[i], HIGH);
    delay(100);
    for (uint i = 0; i < 4; i++) digitalWrite(LED[i], LOW);
    delay(100);
  }
}

void loop() {
  unsigned int tick = 0;
  while (1) {
    if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdTRUE) {
      dac_output_voltage(DAC_CHAN_0, static_cast<uint8_t>(127 + 127 * sin(2 * PI * tick * 440 / 48000.0)));
      // dac_output_voltage(DAC_CHAN_0, tick & 0x4 ? 0 : 255);
      tick++;
    }
  }
}

void uiLoop(void*) {
  while (1) {
    set_ui_state();
    apply_ui_state();
    taskYIELD();
  }
}

void set_ui_state() {
  for (uint i = 0; i < NCTRLS; i++) {
    uint btn_tmp = digitalRead(BTN[i]);
    uint now = millis();
    if (BTN_LAST[i] == 0 || BTN_TMP[i] != btn_tmp) {
      BTN_TMP[i] = btn_tmp;
      BTN_LAST[i] = now;
    } else if (now - BTN_LAST[i] > BTN_DEBOUNCE && BTN_TMP[i] != BTN_STATE[i]) {
      BTN_STATE[i] = BTN_TMP[i];
      // button change event here if needed
    }
    uint pot_tmp = analogRead(POT[i]);
    POT_EMA[i] = (POT_EMA[i] * (POT_EMA_DENOM - POT_EMA_NUM) + (pot_tmp << POT_EMA_XBITS) * POT_EMA_NUM) >> POT_EMA_BITS;
    POT_STATE[i] = POT_EMA[i] >> POT_EMA_XBITS;
  }
}

void apply_ui_state() {

}