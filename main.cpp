
import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"
#include "model.h"

int main() {
    Sine s1 = Sine();
    Sine s2 = Sine(0.01);
    Triangle t = Triangle();
    Square q = Square();
    QuarterWtable w = InterpQWtable(s1, q, 0.5);
    Saw sw1 = Saw(0.5);
    Saw sw2 = Saw(0);
    Saw sw3 = Saw(1);
    WhiteNoise n = WhiteNoise();
    Oscillator car = Oscillator(w, unit_amp, 440, unit_mult);
    Oscillator mod = Oscillator(s1, unit_amp, 660, unit_mult);
    MixedFM fm = MixedFM(car, mod, unit_amp, wet_bal);

    for (int i = 0; i < 1000; i++) {
      cout << i << " " << s1.next(i*440, 0) << endl;
      cout << i << " " << s2.next(i*440, 0) << endl;
      //cout << i << " " << sw2.next(i*100, 0) << endl;
      //cout << i << " " << sw2.next(i*100, 0) << endl;
      // cout << i << " " << fm.next(i, 0) << endl;
    }
}
