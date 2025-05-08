
import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"
#include "model.h"

int main() {
    Wavetable w = Wavetable();
    Oscillator car = Oscillator(w, unit_amp, 440, unit_mult);
    Oscillator mod = Oscillator(w, unit_amp, 660, unit_mult);
    MixedFM fm = MixedFM(car, mod, unit_amp, wet_bal);

    for (int i = 0; i < 1000; i++) {
      cout << i << " " << fm.next(i, 0) << endl;
    }
}
