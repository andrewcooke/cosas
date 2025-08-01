
#ifndef WEAS_LEDS_DIRECT_H
#define WEAS_LEDS_DIRECT_H


#include "pico/types.h"
#include "weas/leds.h"


// add fancy crap to leds


class LEDsDirect {

public:

  LEDsDirect() = default;

  void display12bits(uint16_t v);  // binary, 4 levels of brightness in each led
  void display12bits(int16_t v);  // adds offset to make all positive
  void column10levels(uint c, uint8_t v);  // level meter, 4 brightness each led
  void column3levels(uint c, uint8_t v);   // 1 brightness each led
  void display7levels(uint8_t n);  // snake, 1 brightness
  void columns12bits(uint16_t v);  // both cols, 4 brightness (inexact)
  void columns12bits(int16_t v);  // cols descend for -ve
  void display13levels(uint8_t v);  // head of snake with inversion
  void sq4(uint n, uint8_t v);
  void v2(uint n, uint8_t v);
  void v3(uint n, uint8_t v);
  void h2(uint n, uint8_t v);
  void display7bits(int16_t v);  // binary, -ve as dim

  void set(uint index, uint8_t b) {leds.set(index, b);};
  void set(uint index, uint b) {leds.set(index, b);};
  void set(uint index, bool x) {leds.set(index, x);};
  void on(uint index) {leds.on(index);};
  void off(uint index) {leds.off(index);};
  void all(uint8_t x) {leds.all(x);};
  void all(uint x) {leds.all(x);};
  void all(bool x) {leds.all(x);};

private:

  LEDs& leds = LEDs::get();
  void columns11bits(bool up, uint16_t v);
  void column10levels(bool up, uint c, uint8_t v);

};


#endif

