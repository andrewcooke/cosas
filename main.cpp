
#include <iostream>

#include "constants.h"
#include "source.h"
#include "oscillator.h"
#include "model.h"

int main() {
    Wavetable w = Wavetable();
    Oscillator car = Oscillator(w, unit_amp, 440, unit_mult);
    Oscillator mod = Oscillator(w, unit_amp, 660, unit_mult);
    FM fm = FM(car, mod, unit_amp, wet_bal);

    std::cout << "starting" << std::endl;
    fm.next(0, 0);
    std::cout << (uint16_t)3 << std::endl;
    for (int i = 0; i < 1000; i++) {
      std::cout << i << " " << fm.next(i, 0) << std::endl;
    }
}
