
import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"
#include "model.h"

int main() {
    Sine s = Sine();
    Triangle t = Triangle();
    Square q = Square();
    Wavetable w = InterpWtable(s, q, 0.5);
    Oscillator car = Oscillator(w, unit_amp, 440, unit_mult);
    Oscillator mod = Oscillator(s, unit_amp, 660, unit_mult);
    MixedFM fm = MixedFM(car, mod, unit_amp, wet_bal);

    for (int i = 0; i < 1000; i++) {
      cout << i << " " << s.next(i*100, 0) << endl;
      cout << i << " " << q.next(i*100, 0) << endl;
      cout << i << " " << w.next(i*100, 0) << endl;
      // cout << i << " " << fm.next(i, 0) << endl;
    }
}
