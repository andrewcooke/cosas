
#ifndef WEAS_LEDS_H
#define WEAS_LEDS_H

#include <cstdint>
#include <sys/types.h>


// (unlike ComputerCard) the raw input for these is 8 bits

class LEDs {

public:
    static constexpr uint N = 6;
    LEDs(const LEDs&) = delete;
    LEDs& operator=(const LEDs&) = delete;
    static LEDs& get();
    void set(uint index, uint8_t b);
    void set(uint index, bool x);
    void on(uint index);
    void off(uint index);
    void all(uint8_t x);
    void all(uint x);
    void all(bool x);
    void display12bits(uint16_t v);
    void display12bits(int16_t v);
    void column10levels(uint c, uint8_t v);
    void column3levels(uint c, uint8_t v);
    void display7levels(uint8_t n);
    void columns12bits(uint16_t v);
    void columns12bits(int16_t v);
    void display13levels(uint8_t v);
    void sq4(uint n, uint8_t v);
    void v2(uint n, uint8_t v);
    void v3(uint n, uint8_t v);
    void h2(uint n, uint8_t v);

private:
    LEDs();
    static constexpr uint LED_BASE_GPIO = 10;
    void columns11bits(bool up, uint16_t v);
    void column10levels(bool up, uint c, uint8_t v);
};


#endif
