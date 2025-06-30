
#include "CppUTest/TestHarness.h"

#include "cosas/oscillator.h"


TEST_GROUP(Oscillator)


TEST(Oscillator, Wavedex) {
  Wavelib w = Wavelib();
  AbsDexOsc o = AbsDexOsc(w, w.sine_gamma_1, 4400);
  CHECK(o.next(1000, 0) == 5800);
  o.get_dex().set(w.square_duty_05);
  CHECK(o.next(1000, 0) == 32767);
}


TEST(Oscillator, Detune) {
  Wavelib w = Wavelib();
  AbsDexOsc o1 = AbsDexOsc(w, w.sine_gamma_1, 4400);
  RelDexOsc o2 = RelDexOsc(w, w.sine_gamma_1, o1.get_freq(), 1, 1);
  CHECK(o2.next(1000, 0) == 5800);
  o2.get_freq().get_det().set(0.9);
  CHECK(o2.next(1000, 0) != 5800);
}


TEST(Oscillator, AbsPolyOsc) {
  AbsPolyOsc o = AbsPolyOsc(440, PolyTable::sine, 0, quarter_table_size);
  CHECK(o.next(0, 0) == 0);
  // there are table_size samples a second.  a 440hz note takes 1/440s
  // for one cycle, which is table_size/440 samples.  so a quarter of
  // a cycle takes quarter_table_size/440. 
  CHECK(o.next((quarter_table_size / 440) << subtick_bits, 0) == sample_max - 1);  // almost
}
