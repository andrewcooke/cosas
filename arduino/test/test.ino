
#include "driver/dac.h"
#include "math.h"

const int POT_1 = 13;
const int POT_2 = 14;
const int POT_3 = 12;
const int POT_4 = 27;

const int BTN_1 = 18;
const int BTN_2 = 19;
const int BTN_3 = 15;
const int BTN_4 = 4;

const int LEDS[4] = {23, 32, 5, 2};

void setup() {

  esp_log_level_set("*", ESP_LOG_NONE);
  
  pinMode(POT_1, INPUT);
  pinMode(POT_2, INPUT);
  pinMode(POT_3, INPUT);
  pinMode(POT_4, INPUT);
  
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  
  for (int i = 0; i < 4; i++) {
    pinMode(LEDS[i], OUTPUT);
    digitalWrite(LEDS[i], LOW);
  }
  
  dac_output_enable(DAC_CHAN_0);
  dac_output_voltage(DAC_CHAN_0, 128); // Silencio inicial
  
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(LEDS[i], HIGH);
    }
    delay(100);
    for (int i = 0; i < 4; i++) {
      digitalWrite(LEDS[i], LOW);
    }
    delay(100);
  }
}

int tick = 0;

void loop() {
  dac_output_voltage(DAC_CHAN_0, static_cast<uint8_t>(127 + 127 * sin(tick / 1000.0)));
  tick++;
}

