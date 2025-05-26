
#include <memory>

#include "engine.h"
#include "modulators.h"
#include "constants.h"
#include "console.h"
//#include "wavetable.h"

int main() {
  //  Square s = Square(0.25);
  //  Sine s = Sine(1);
  //  Triangle s = Triangle();
  //  WTriangle s = WTriangle();
  //  WSaw s = WSaw(0.5);
  //  Saw s = Saw(0.5);
  //  dump(s, 2 * sample_rate);
  //  Oscillator o = Oscillator(s, std::move(std::make_unique<AbsoluteFreq>(440)));
  //  dump(o, 0.1 * sample_rate);
  //  Wavelib w = Wavelib();
  //  Wavedex wd = Wavedex(w, w.sine_gamma_1);
  //  AbsoluteFreq f = AbsoluteFreq(440);
  //  Oscillator o = Oscillator(wd, std::move(std::make_unique<RelativeFreq>(f, 1)));
  //  Oscillator o = Oscillator(wd, std::move(std::make_unique<AbsoluteFreq>(440)));
  //  Gain g = Gain(o, 0.01);
  //  dump(g, 0.01 * sample_rate);
  //  Noise s = Noise(16);
  //  dump(s, 0.1 * sample_rate);
  Manager mgr = Manager();
  const Node& nd = mgr.build(Manager::Engine::FM_FB_FLT);
  dump(nd, 0.01 * sample_rate);
  //  Manager mgr = Manager();
  //  dump(mgr.build_fm_mod(1), 0.1 * sample_rate);
  //  dump(mgr.build_simple_fm(1), 0.01 * sample_rate);
};
