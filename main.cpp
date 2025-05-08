
import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"
#include "model.h"

int main() {

  int root = 440;
  Square q1 = Square();
  Sine s1 = Sine();
  Oscillator o1 = Oscillator(q1, unit_amp, root);
  Multiplier m = Multiplier(1, 80);
  Amplitude a = Amplitude(1);
  Oscillator o2 = Oscillator(s1, a, root, m);
  MixedFM fm = MixedFM(o1, o2, unit_amp, wet_bal);

  Square q2 = Square();
  Oscillator o3 = Oscillator(q2, unit_amp, root);
  Balance b = Balance(0.5);
  Mixer x = Mixer(fm, o3, unit_amp, b);
  
  for (int i = 0; i < 1000; i++) {
    cout << i << " " << x.next(i, 0) << endl;
  }
}
