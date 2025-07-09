
#include "cosas/AudioBuffer.h"

#include "pico/time.h"

// just saved here for now

class BufferWea : public AudioBuffer {

protected:

    void sleep_us(uint32_t us) override {
        ::sleep_us(us);
    }

    void log_delta(int d) override {
        led.display7levels(d);
    }

private:

    LED& led = LED::get();

};