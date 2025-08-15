
#ifndef WEAS_LEDS_H
#define WEAS_LEDS_H

#include <pico/types.h>


// an interface to the LEDs
// (unlike ComputerCard) the raw input for these is 8 bits
// indices are top left 0, top right 1, bottom left 4.


class LEDs {

public:

    static constexpr uint N = 6;

    LEDs(const LEDs&) = delete;
    LEDs& operator=(const LEDs&) = delete;
    static LEDs& get();

    void set(uint index, uint8_t b);
    void set(uint index, uint b);  // alias for literals - forwards to uint8_t
    void set(uint index, bool x);
    void on(uint index);
    void off(uint index);
    void all(uint8_t x);
    void all(uint x);  // for numeric literals - forwards to uint8_t
    void all(bool x);

private:

    LEDs();
    static constexpr uint LED_BASE_GPIO = 10;

};


#endif
