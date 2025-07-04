
#ifndef WEA_LEDS_H
#define WEA_LEDS_H

#include <cstdint>
#include <sys/types.h>


class LEDs {
private:
    LEDs();
    static constexpr uint BASE_GPIO = 10;
public:
    // singleton - prevent copying
    LEDs(const LEDs&) = delete;
    LEDs& operator=(const LEDs&) = delete;
    static LEDs& get();
    void set(uint index, uint8_t b);
    void set(uint index, bool x);
    void all(uint8_t x);
    void all(bool x);
    void display12bits(uint16_t v);
    void display12bits(int16_t v);
    void column10levels(uint c, uint8_t v);
};


#endif
