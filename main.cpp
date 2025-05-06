
import std;
using namespace std;

#include "constants.h"
#include "oscillator.h"

int main() {
    Wavetable w = Wavetable();
    StepScale scale = StepScale(30, 12);
    uint16_t step = sample_rate / 440;
    uint16_t tick = 0;
    for (uint16_t i = 0; i < 1000; i++) {
        // cout << tick << " " << w.at_uint16_t(tick, scale) << endl;
        cout << i << " " << w.at_float(tick, scale) << endl;
        tick = (tick + step) % sample_rate;
    }
}
