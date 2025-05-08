
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
    QuarterWtable w = InterpQWtable(s, q, 0.5);
    Saw sw1 = Saw(0.5);
    Saw sw2 = Saw(0);
    Saw sw3 = Saw(1);
    Oscillator car = Oscillator(w, unit_amp, 440, unit_mult);
    Oscillator mod = Oscillator(s, unit_amp, 660, unit_mult);
    MixedFM fm = MixedFM(car, mod, unit_amp, wet_bal);

    for (int i = 0; i < 1000; i++) {
      cout << i << " " << sw1.next(i*100, 0) << endl;
      //cout << i << " " << sw2.next(i*100, 0) << endl;
      //cout << i << " " << sw2.next(i*100, 0) << endl;
      // cout << i << " " << fm.next(i, 0) << endl;
    }
}
