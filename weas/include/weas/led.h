
#ifndef WEA_LED_H
#define WEA_LED_H

#include <cstdint>
#include <sys/types.h>


class LED {

public:
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
    void display7levels(uint8_t n);
    void columns12bits(uint16_t v);

private:
    LED();
    static constexpr uint LED_BASE_GPIO = 10;
    static constexpr uint LED_N = 6;
};


#endif
