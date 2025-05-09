
import std;
using namespace std;

#include "constants.h"
#include "source.h"
#include "oscillator.h"
#include "engine.h"


int main() {

  Manager manager;
  Oscillator& o0 = manager.get_oscillator(0);
  Oscillator& o1 = manager.get_oscillator(1);
  MixedFM fm = MixedFM(o0, o1, unit_amp, wet_bal);

  for (int i = 0; i < 1000; i++) {
    cout << i << " " << fm.next(i, 0) << endl;
  }
  
  
  /*
  int root = 440;

  Frequency f1 = Frequency(root);
  Sine s1 = Sine();
  Oscillator o1 = Oscillator(s1, unit_amp, f1);

  Sine s2 = Sine();
  Frequency f2 = Frequency(root, 1, 80);
  Oscillator o2 = Oscillator(s2, unit_amp, f2);
  MixedFM fm = MixedFM(o1, o2, unit_amp, wet_bal);

  Square q3 = Square(0.25);
  Frequency f3 = Frequency(root, 2, 3);
  Oscillator o3 = Oscillator(q3, unit_amp, f3);
  Balance b3 = Balance(0.5);
  Mixer x = Mixer(fm, o3, unit_amp, b3);
  
  for (int i = 0; i < 1000; i++) {
    cout << i << " " << x.next(i, 0) << endl;
  }
  */
}
