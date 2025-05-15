
#include "constants.h"
#include "console.h"
#include "lookup.h"

int main() {
  //  Square s = Square(0.25);
  //  Sine s = Sine(1);
  //  Triangle s = Triangle();
  //  WTriangle s = WTriangle();
  //  WSaw s = WSaw(0.5);
  //  Saw s = Saw(0.5);
  //  dump(s, 2 * sample_rate);
  Noise s = Noise(16);
  dump(s, 0.1 * sample_rate);
};
