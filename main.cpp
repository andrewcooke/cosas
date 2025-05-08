
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
    Square q1 = Square();
    Square q2 = Square(0.1);
    Saw sw1 = Saw(0.5);
    Saw sw2 = Saw(0);
    Saw sw3 = Saw(1);
    Noise n1 = Noise();
    Noise n2 = Noise(5);
    Oscillator car = Oscillator(s1, unit_amp, 440, unit_mult);
    Oscillator mod = Oscillator(s2, unit_amp, 660, unit_mult);
    MixedFM fm = MixedFM(car, mod, unit_amp, wet_bal);

    for (int i = 0; i < 1000; i++) {
      //cout << i << " " << q1.next(i*440, 0) << endl;
      //cout << i << " " << n1.next(i, 0) << endl;
      cout << i << " " << n2.next(i, 0) << endl;
      //cout << i << " " << sw2.next(i*100, 0) << endl;
      //cout << i << " " << sw2.next(i*100, 0) << endl;
      // cout << i << " " << fm.next(i, 0) << endl;
    }
}
