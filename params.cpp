
#include <algorithm>
#include <iostream>

#include "constants.h"
#include "maths.h"
#include "params.h"
#include "source.h"
#include "oscillator.h"


Amplitude::Amplitude(float a) : amplitude(a) {}

Amplitude::Amplitude() : Amplitude(1) {}

int16_t Amplitude::scale(int16_t amp) const {
  float a = amplitude * amp;
  int16_t b = clip_16(a);
  std::cerr << amp << " amp " << this << " (" << amplitude << ") " << a << " " << b << std::endl;
  return b;
};

void Amplitude::set(float f) {
  std::cerr << amplitude << " amp " << this << " set " << f << std::endl;
  amplitude = f;
}

TEST_CASE("Amplitude") {
  Amplitude a = Amplitude();
  CHECK(a.scale(110) == 110);
  a.set(0.1);
  CHECK(a.scale(110) == 11);
}


Balance::Balance() : Balance(1) {}

Balance::Balance(float w) : wet_weight(w) {}

int16_t Balance::combine(int16_t wet, int16_t dry) const {
  return clip_16(wet_weight * wet + (1 - wet_weight) * dry);
}


//Wavedex::Wavedex(Wavelib& wl, size_t idx) : wavelib(wl), wavedex(idx), wavetable(wl[idx]) {}

/*
void Wavedex::set_wavedex(float idx) {
  size_t n = wavelib.size() - 1;
  wavedex = std::max(static_cast<size_t>(0), std::min(n, static_cast<size_t>(idx * n)));
  wavetable = wavelib[wavedex];
}
*/

//Wavetable& Wavedex::get_wavetable() const {
//  return wavetable;
//}

  
