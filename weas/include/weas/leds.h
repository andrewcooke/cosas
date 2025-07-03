
#ifndef WEA_LEDS_H
#define WEA_LEDS_H

#include <cstdint>
#include <sys/types.h>


class LEDs {
private:
    LEDs();
    // singleton - prevent copying
    LEDs(const LEDs&) = delete;
    LEDs& operator=(const LEDs&) = delete;
    static constexpr uint BASE_GPIO = 10;
public:
    static LEDs& get();
    void set(uint index, uint8_t b);
};


#endif
