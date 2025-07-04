
#ifndef WEA_LED_H
#define WEA_LED_H

#include <cstdint>
#include <sys/types.h>


class LED {
private:
    LED();
    static constexpr uint BASE_GPIO = 10;
public:
    // singleton - prevent copying
    LED(const LED&) = delete;
    LED& operator=(const LED&) = delete;
    static LED& get();
    void set(uint index, uint8_t b);
    void set(uint index, bool x);
    void all(uint8_t x);
    void all(bool x);
    void display12bits(uint16_t v);
    void display12bits(int16_t v);
    void column10levels(uint c, uint8_t v);
};


#endif
